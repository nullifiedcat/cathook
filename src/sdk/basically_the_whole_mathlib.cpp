// This file just exists because the setupbones reconstruction needs so so
// many Mathlib functions, and i do not want to clutter helpers.cpp
#include "common.hpp"

void AngleMatrix(const QAngle &angles, matrix3x4_t &matrix)
{
#ifdef _VPROF_MATHLIB
    VPROF_BUDGET("AngleMatrix", "Mathlib");
#endif
    Assert(s_bMathlibInitialized);

    float sr, sp, sy, cr, cp, cy;

#ifdef _X360
    fltx4 radians, scale, sine, cosine;
    radians = LoadUnaligned3SIMD(angles.Base());
    scale   = ReplicateX4(M_PI_F / 180.f);
    radians = MulSIMD(radians, scale);
    SinCos3SIMD(sine, cosine, radians);

    sp = SubFloat(sine, 0);
    sy = SubFloat(sine, 1);
    sr = SubFloat(sine, 2);
    cp = SubFloat(cosine, 0);
    cy = SubFloat(cosine, 1);
    cr = SubFloat(cosine, 2);
#else
    SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
    SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
    SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);
#endif

    // matrix = (YAW * PITCH) * ROLL
    matrix[0][0] = cp * cy;
    matrix[1][0] = cp * sy;
    matrix[2][0] = -sp;

    float crcy   = cr * cy;
    float crsy   = cr * sy;
    float srcy   = sr * cy;
    float srsy   = sr * sy;
    matrix[0][1] = sp * srcy - crsy;
    matrix[1][1] = sp * srsy + crcy;
    matrix[2][1] = sr * cp;

    matrix[0][2] = (sp * crcy + srsy);
    matrix[1][2] = (sp * crsy - srcy);
    matrix[2][2] = cr * cp;

    matrix[0][3] = 0.0f;
    matrix[1][3] = 0.0f;
    matrix[2][3] = 0.0f;
}

void MatrixSetColumn(const Vector &in, int column, matrix3x4_t &out)
{
    out[0][column] = in.x;
    out[1][column] = in.y;
    out[2][column] = in.z;
}

void AngleMatrix(const QAngle &angles, const Vector &position, matrix3x4_t &matrix)
{
    AngleMatrix(angles, matrix);
    MatrixSetColumn(position, 3, matrix);
}

void AngleMatrix(const RadianEuler &angles, matrix3x4_t &matrix)
{
    QAngle quakeEuler(RAD2DEG(angles.y), RAD2DEG(angles.z), RAD2DEG(angles.x));

    AngleMatrix(quakeEuler, matrix);
}

void AngleMatrix(RadianEuler const &angles, const Vector &position, matrix3x4_t &matrix)
{
    AngleMatrix(angles, matrix);
    MatrixSetColumn(position, 3, matrix);
}

void QuaternionSlerp(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt)
{
    Quaternion q2;
    // 0.0 returns p, 1.0 return q.

    // decide if one of the quaternions is backwards
    QuaternionAlign(p, q, q2);

    QuaternionSlerpNoAlign(p, q2, t, qt);
}

void QuaternionAngles(const Quaternion &q, RadianEuler &angles)
{
    Assert(s_bMathlibInitialized);
    Assert(q.IsValid());

    // FIXME: doing it this way calculates too much data, needs to do an optimized version...
    matrix3x4_t matrix;
    QuaternionMatrix(q, matrix);
    MatrixAngles(matrix, angles);

    Assert(angles.IsValid());
}

void QuaternionAlign(const Quaternion &p, const Quaternion &q, Quaternion &qt)
{
    Assert(s_bMathlibInitialized);

    // FIXME: can this be done with a quat dot product?

    int i;
    // decide if one of the quaternions is backwards
    float a = 0;
    float b = 0;
    for (i = 0; i < 4; i++)
    {
        a += (p[i] - q[i]) * (p[i] - q[i]);
        b += (p[i] + q[i]) * (p[i] + q[i]);
    }
    if (a > b)
    {
        for (i = 0; i < 4; i++)
        {
            qt[i] = -q[i];
        }
    }
    else if (&qt != &q)
    {
        for (i = 0; i < 4; i++)
        {
            qt[i] = q[i];
        }
    }
}

float QuaternionNormalize(Quaternion &q)
{
    Assert(s_bMathlibInitialized);
    float radius, iradius;

    Assert(q.IsValid());

    radius = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];

    if (radius) // > FLT_EPSILON && ((radius < 1.0f - 4*FLT_EPSILON) || (radius > 1.0f + 4*FLT_EPSILON))
    {
        radius  = sqrt(radius);
        iradius = 1.0f / radius;
        q[3] *= iradius;
        q[2] *= iradius;
        q[1] *= iradius;
        q[0] *= iradius;
    }
    return radius;
}

void QuaternionMatrix(const Quaternion &q, const Vector &pos, matrix3x4_t &matrix)
{
    if (!HushAsserts())
    {
        Assert(pos.IsValid());
    }

    QuaternionMatrix(q, matrix);

    matrix[0][3] = pos.x;
    matrix[1][3] = pos.y;
    matrix[2][3] = pos.z;
}

const int32 ALIGN16 g_SIMD_ComponentMask[4][4] ALIGN16_POST = { { static_cast<int32>(0xFFFFFFFF), 0, 0, 0 }, { 0, static_cast<int32>(0xFFFFFFFF), 0, 0 }, { 0, 0, static_cast<int32>(0xFFFFFFFF), 0 }, { 0, 0, 0, static_cast<int32>(0xFFFFFFFF) } };
void ConcatTransforms(const matrix3x4_t &in1, const matrix3x4_t &in2, matrix3x4_t &out)
{
#if 0
    // test for ones that'll be 2x faster
    if ( (((size_t)&in1) % 16) == 0 && (((size_t)&in2) % 16) == 0 && (((size_t)&out) % 16) == 0 )
    {
        ConcatTransforms_Aligned( in1, in2, out );
        return;
    }
#endif

    fltx4 lastMask = *(fltx4 *) (&g_SIMD_ComponentMask[3]);
    fltx4 rowA0    = LoadUnalignedSIMD(in1.m_flMatVal[0]);
    fltx4 rowA1    = LoadUnalignedSIMD(in1.m_flMatVal[1]);
    fltx4 rowA2    = LoadUnalignedSIMD(in1.m_flMatVal[2]);

    fltx4 rowB0 = LoadUnalignedSIMD(in2.m_flMatVal[0]);
    fltx4 rowB1 = LoadUnalignedSIMD(in2.m_flMatVal[1]);
    fltx4 rowB2 = LoadUnalignedSIMD(in2.m_flMatVal[2]);

    // now we have the rows of m0 and the columns of m1
    // first output row
    fltx4 A0    = SplatXSIMD(rowA0);
    fltx4 A1    = SplatYSIMD(rowA0);
    fltx4 A2    = SplatZSIMD(rowA0);
    fltx4 mul00 = MulSIMD(A0, rowB0);
    fltx4 mul01 = MulSIMD(A1, rowB1);
    fltx4 mul02 = MulSIMD(A2, rowB2);
    fltx4 out0  = AddSIMD(mul00, AddSIMD(mul01, mul02));

    // second output row
    A0          = SplatXSIMD(rowA1);
    A1          = SplatYSIMD(rowA1);
    A2          = SplatZSIMD(rowA1);
    fltx4 mul10 = MulSIMD(A0, rowB0);
    fltx4 mul11 = MulSIMD(A1, rowB1);
    fltx4 mul12 = MulSIMD(A2, rowB2);
    fltx4 out1  = AddSIMD(mul10, AddSIMD(mul11, mul12));

    // third output row
    A0          = SplatXSIMD(rowA2);
    A1          = SplatYSIMD(rowA2);
    A2          = SplatZSIMD(rowA2);
    fltx4 mul20 = MulSIMD(A0, rowB0);
    fltx4 mul21 = MulSIMD(A1, rowB1);
    fltx4 mul22 = MulSIMD(A2, rowB2);
    fltx4 out2  = AddSIMD(mul20, AddSIMD(mul21, mul22));

    // add in translation vector
    A0   = AndSIMD(rowA0, lastMask);
    A1   = AndSIMD(rowA1, lastMask);
    A2   = AndSIMD(rowA2, lastMask);
    out0 = AddSIMD(out0, A0);
    out1 = AddSIMD(out1, A1);
    out2 = AddSIMD(out2, A2);

    // write to output
    StoreUnalignedSIMD(out.m_flMatVal[0], out0);
    StoreUnalignedSIMD(out.m_flMatVal[1], out1);
    StoreUnalignedSIMD(out.m_flMatVal[2], out2);
}

void MatrixAngles(const matrix3x4_t &matrix, Quaternion &q, Vector &pos)
{
#ifdef _VPROF_MATHLIB
    VPROF_BUDGET("MatrixQuaternion", "Mathlib");
#endif
    float trace;
    trace = matrix[0][0] + matrix[1][1] + matrix[2][2] + 1.0f;
    if (trace > 1.0f + FLT_EPSILON)
    {
        // VPROF_INCREMENT_COUNTER("MatrixQuaternion A",1);
        q.x = (matrix[2][1] - matrix[1][2]);
        q.y = (matrix[0][2] - matrix[2][0]);
        q.z = (matrix[1][0] - matrix[0][1]);
        q.w = trace;
    }
    else if (matrix[0][0] > matrix[1][1] && matrix[0][0] > matrix[2][2])
    {
        // VPROF_INCREMENT_COUNTER("MatrixQuaternion B",1);
        trace = 1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2];
        q.x   = trace;
        q.y   = (matrix[1][0] + matrix[0][1]);
        q.z   = (matrix[0][2] + matrix[2][0]);
        q.w   = (matrix[2][1] - matrix[1][2]);
    }
    else if (matrix[1][1] > matrix[2][2])
    {
        // VPROF_INCREMENT_COUNTER("MatrixQuaternion C",1);
        trace = 1.0f + matrix[1][1] - matrix[0][0] - matrix[2][2];
        q.x   = (matrix[0][1] + matrix[1][0]);
        q.y   = trace;
        q.z   = (matrix[2][1] + matrix[1][2]);
        q.w   = (matrix[0][2] - matrix[2][0]);
    }
    else
    {
        // VPROF_INCREMENT_COUNTER("MatrixQuaternion D",1);
        trace = 1.0f + matrix[2][2] - matrix[0][0] - matrix[1][1];
        q.x   = (matrix[0][2] + matrix[2][0]);
        q.y   = (matrix[2][1] + matrix[1][2]);
        q.z   = trace;
        q.w   = (matrix[1][0] - matrix[0][1]);
    }

    QuaternionNormalize(q);

#if 0
    // check against the angle version
    RadianEuler ang;
    MatrixAngles( matrix, ang );
    Quaternion test;
    AngleQuaternion( ang, test );
    float d = QuaternionDotProduct( q, test );
    Assert( fabs(d) > 0.99 && fabs(d) < 1.01 );
#endif

    MatrixGetColumn(matrix, 3, pos);
}

// assume in2 is a rotation and rotate the input vector
void VectorRotate(const float *in1, const matrix3x4_t &in2, float *out)
{
    Assert(s_bMathlibInitialized);
    Assert(in1 != out);
    out[0] = DotProduct(in1, in2[0]);
    out[1] = DotProduct(in1, in2[1]);
    out[2] = DotProduct(in1, in2[2]);
}

void QuaternionScale(const Quaternion &p, float t, Quaternion &q)
{
    Assert(s_bMathlibInitialized);

#if 0
    Quaternion p0;
    Quaternion q;
    p0.Init( 0.0, 0.0, 0.0, 1.0 );

    // slerp in "reverse order" so that p doesn't get realigned
    QuaternionSlerp( p, p0, 1.0 - fabs( t ), q );
    if (t < 0.0)
    {
        q.w = -q.w;
    }
#else
    float r;

    // FIXME: nick, this isn't overly sensitive to accuracy, and it may be faster to
    // use the cos part (w) of the quaternion (sin(omega)*N,cos(omega)) to figure the new scale.
    float sinom = sqrt(DotProduct(&p.x, &p.x));
    sinom       = min(sinom, 1.f);

    float sinsom = sin(asin(sinom) * t);

    t = sinsom / (sinom + FLT_EPSILON);
    VectorScale(&p.x, t, &q.x);

    // rescale rotation
    r = 1.0f - sinsom * sinsom;

    // Assert( r >= 0 );
    if (r < 0.0f)
        r = 0.0f;
    r = sqrt(r);

    // keep sign of rotation
    if (p.w < 0)
        q.w = -r;
    else
        q.w = r;
#endif

    Assert(q.IsValid());

    return;
}

// qt = p * q
void QuaternionMult(const Quaternion &p, const Quaternion &q, Quaternion &qt)
{
    Assert(s_bMathlibInitialized);
    Assert(p.IsValid());
    Assert(q.IsValid());

    if (&p == &qt)
    {
        Quaternion p2 = p;
        QuaternionMult(p2, q, qt);
        return;
    }

    // decide if one of the quaternions is backwards
    Quaternion q2;
    QuaternionAlign(p, q, q2);

    qt.x = p.x * q2.w + p.y * q2.z - p.z * q2.y + p.w * q2.x;
    qt.y = -p.x * q2.z + p.y * q2.w + p.z * q2.x + p.w * q2.y;
    qt.z = p.x * q2.y - p.y * q2.x + p.z * q2.w + p.w * q2.z;
    qt.w = -p.x * q2.x - p.y * q2.y - p.z * q2.z + p.w * q2.w;
}

//-----------------------------------------------------------------------------
// Purpose: Converts radian-euler axis aligned angles to a quaternion
// Input  : *pfAngles - Right-handed Euler angles in radians
//			*outQuat - quaternion of form (i,j,k,real)
//-----------------------------------------------------------------------------
void AngleQuaternion(const RadianEuler &angles, Quaternion &outQuat)
{
    Assert(s_bMathlibInitialized);
    //	Assert( angles.IsValid() );

#ifdef _VPROF_MATHLIB
    VPROF_BUDGET("AngleQuaternion", "Mathlib");
#endif

    float sr, sp, sy, cr, cp, cy;

#ifdef _X360
    fltx4 radians, scale, sine, cosine;
    radians = LoadUnaligned3SIMD(&angles.x);
    scale   = ReplicateX4(0.5f);
    radians = MulSIMD(radians, scale);
    SinCos3SIMD(sine, cosine, radians);

    // NOTE: The ordering here is *different* from the AngleQuaternion below
    // because p, y, r are not in the same locations in QAngle + RadianEuler. Yay!
    sr = SubFloat(sine, 0);
    sp = SubFloat(sine, 1);
    sy = SubFloat(sine, 2);
    cr = SubFloat(cosine, 0);
    cp = SubFloat(cosine, 1);
    cy = SubFloat(cosine, 2);
#else
    SinCos(angles.z * 0.5f, &sy, &cy);
    SinCos(angles.y * 0.5f, &sp, &cp);
    SinCos(angles.x * 0.5f, &sr, &cr);
#endif

    // NJS: for some reason VC6 wasn't recognizing the common subexpressions:
    float srXcp = sr * cp, crXsp = cr * sp;
    outQuat.x = srXcp * cy - crXsp * sy; // X
    outQuat.y = crXsp * cy + srXcp * sy; // Y

    float crXcp = cr * cp, srXsp = sr * sp;
    outQuat.z = crXcp * sy - srXsp * cy; // Z
    outQuat.w = crXcp * cy + srXsp * sy; // W (real component)
}

// NOTE: This is just the transpose not a general inverse
void MatrixInvert(const matrix3x4_t &in, matrix3x4_t &out)
{
    Assert(s_bMathlibInitialized);
    if (&in == &out)
    {
        V_swap(out[0][1], out[1][0]);
        V_swap(out[0][2], out[2][0]);
        V_swap(out[1][2], out[2][1]);
    }
    else
    {
        // transpose the matrix
        out[0][0] = in[0][0];
        out[0][1] = in[1][0];
        out[0][2] = in[2][0];

        out[1][0] = in[0][1];
        out[1][1] = in[1][1];
        out[1][2] = in[2][1];

        out[2][0] = in[0][2];
        out[2][1] = in[1][2];
        out[2][2] = in[2][2];
    }

    // now fix up the translation to be in the other space
    float tmp[3];
    tmp[0] = in[0][3];
    tmp[1] = in[1][3];
    tmp[2] = in[2][3];

    out[0][3] = -DotProduct(tmp, out[0]);
    out[1][3] = -DotProduct(tmp, out[1]);
    out[2][3] = -DotProduct(tmp, out[2]);
}

void QuaternionBlend(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt)
{
    Assert(s_bMathlibInitialized);
#if ALLOW_SIMD_QUATERNION_MATH
    fltx4 psimd, qsimd, qtsimd;
    psimd  = LoadUnalignedSIMD(p.Base());
    qsimd  = LoadUnalignedSIMD(q.Base());
    qtsimd = QuaternionBlendSIMD(psimd, qsimd, t);
    StoreUnalignedSIMD(qt.Base(), qtsimd);
#else
    // decide if one of the quaternions is backwards
    Quaternion q2;
    QuaternionAlign(p, q, q2);
    QuaternionBlendNoAlign(p, q2, t, qt);
#endif
}

void QuaternionBlendNoAlign(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt)
{
    Assert(s_bMathlibInitialized);
    float sclp, sclq;
    int i;

    // 0.0 returns p, 1.0 return q.
    sclp = 1.0f - t;
    sclq = t;
    for (i = 0; i < 4; i++)
    {
        qt[i] = sclp * p[i] + sclq * q[i];
    }
    QuaternionNormalize(qt);
}

void QuaternionIdentityBlend(const Quaternion &p, float t, Quaternion &qt)
{
    Assert(s_bMathlibInitialized);
    float sclp;

    sclp = 1.0f - t;

    qt.x = p.x * sclp;
    qt.y = p.y * sclp;
    qt.z = p.z * sclp;
    if (qt.w < 0.0)
    {
        qt.w = p.w * sclp - t;
    }
    else
    {
        qt.w = p.w * sclp + t;
    }
    QuaternionNormalize(qt);
}

void QuaternionMatrix(const Quaternion &q, matrix3x4_t &matrix)
{
    Assert(s_bMathlibInitialized);
    if (!HushAsserts())
    {
        Assert(q.IsValid());
    }

#ifdef _VPROF_MATHLIB
    VPROF_BUDGET("QuaternionMatrix", "Mathlib");
#endif

// Original code
// This should produce the same code as below with optimization, but looking at the assmebly,
// it doesn't.  There are 7 extra multiplies in the release build of this, go figure.
#if 1
    matrix[0][0] = 1.0 - 2.0 * q.y * q.y - 2.0 * q.z * q.z;
    matrix[1][0] = 2.0 * q.x * q.y + 2.0 * q.w * q.z;
    matrix[2][0] = 2.0 * q.x * q.z - 2.0 * q.w * q.y;

    matrix[0][1] = 2.0f * q.x * q.y - 2.0f * q.w * q.z;
    matrix[1][1] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.z * q.z;
    matrix[2][1] = 2.0f * q.y * q.z + 2.0f * q.w * q.x;

    matrix[0][2] = 2.0f * q.x * q.z + 2.0f * q.w * q.y;
    matrix[1][2] = 2.0f * q.y * q.z - 2.0f * q.w * q.x;
    matrix[2][2] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.y * q.y;

    matrix[0][3] = 0.0f;
    matrix[1][3] = 0.0f;
    matrix[2][3] = 0.0f;
#else
    float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

    // precalculate common multiplitcations
    x2 = q.x + q.x;
    y2 = q.y + q.y;
    z2 = q.z + q.z;
    xx = q.x * x2;
    xy = q.x * y2;
    xz = q.x * z2;
    yy = q.y * y2;
    yz = q.y * z2;
    zz = q.z * z2;
    wx = q.w * x2;
    wy = q.w * y2;
    wz = q.w * z2;

    matrix[0][0] = 1.0 - (yy + zz);
    matrix[0][1] = xy - wz;
    matrix[0][2] = xz + wy;
    matrix[0][3] = 0.0f;

    matrix[1][0] = xy + wz;
    matrix[1][1] = 1.0 - (xx + zz);
    matrix[1][2] = yz - wx;
    matrix[1][3] = 0.0f;

    matrix[2][0] = xz - wy;
    matrix[2][1] = yz + wx;
    matrix[2][2] = 1.0 - (xx + yy);
    matrix[2][3] = 0.0f;
#endif
}

void QuaternionSlerpNoAlign(const Quaternion &p, const Quaternion &q, float t, Quaternion &qt)
{
    Assert(s_bMathlibInitialized);
    float omega, cosom, sinom, sclp, sclq;
    int i;

    // 0.0 returns p, 1.0 return q.

    cosom = p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3];

    if ((1.0f + cosom) > 0.000001f)
    {
        if ((1.0f - cosom) > 0.000001f)
        {
            omega = acos(cosom);
            sinom = sin(omega);
            sclp  = sin((1.0f - t) * omega) / sinom;
            sclq  = sin(t * omega) / sinom;
        }
        else
        {
            // TODO: add short circuit for cosom == 1.0f?
            sclp = 1.0f - t;
            sclq = t;
        }
        for (i = 0; i < 4; i++)
        {
            qt[i] = sclp * p[i] + sclq * q[i];
        }
    }
    else
    {
        Assert(&qt != &q);

        qt[0] = -q[1];
        qt[1] = q[0];
        qt[2] = -q[3];
        qt[3] = q[2];
        sclp  = sin((1.0f - t) * (0.5f * M_PI));
        sclq  = sin(t * (0.5f * M_PI));
        for (i = 0; i < 3; i++)
        {
            qt[i] = sclp * p[i] + sclq * qt[i];
        }
    }

    Assert(qt.IsValid());
}

const studiohdr_t *virtualgroup_t::GetStudioHdr(void) const
{
    return g_IMDLCache->GetStudioHdr((MDLHandle_t)(uintptr_t) cache & 0xffff);
}
int studiohdr_t::GetAutoplayList(unsigned short **pOut) const
{
    return g_IMDLCache->GetAutoplayList((MDLHandle_t)(uintptr_t) virtualModel & 0xffff, pOut);
}
