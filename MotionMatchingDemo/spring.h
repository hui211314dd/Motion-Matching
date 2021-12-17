#pragma once

#include "common.h"
#include "vec.h"
#include "quat.h"

//--------------------------------------

static inline float damper_implicit(float x, float g, float halflife, float dt, float eps=1e-5f)
{
    return lerpf(x, g, 1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
}

static inline vec3 damper_implicit(vec3 x, vec3 g, float halflife, float dt, float eps=1e-5f)
{
    return lerp(x, g, 1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
}

static inline quat damper_implicit(quat x, quat g, float halflife, float dt, float eps=1e-5f)
{
    return quat_slerp_shortest_approx(x, g, 1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
}

static inline float damp_adjustment_implicit(float g, float halflife, float dt, float eps=1e-5f)
{
    return g * (1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
}

static inline vec3 damp_adjustment_implicit(vec3 g, float halflife, float dt, float eps=1e-5f)
{
    return g * (1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
}

static inline quat damp_adjustment_implicit(quat g, float halflife, float dt, float eps=1e-5f)
{
    return quat_slerp_shortest_approx(quat(), g, 1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
}

//--------------------------------------

static inline float halflife_to_damping(float halflife, float eps = 1e-5f)
{
    return (4.0f * LN2f) / (halflife + eps);
}
    
static inline float damping_to_halflife(float damping, float eps = 1e-5f)
{
    return (4.0f * LN2f) / (damping + eps);
}

static inline float frequency_to_stiffness(float frequency)
{
   return squaref(2.0f * PIf * frequency);
}

static inline float stiffness_to_frequency(float stiffness)
{
    return sqrtf(stiffness) / (2.0f * PIf);
}

//--------------------------------------

static inline void simple_spring_damper_implicit(
    float& x, 
    float& v, 
    const float x_goal, 
    const float halflife, 
    const float dt)
{
    float y = halflife_to_damping(halflife) / 2.0f;	
    float j0 = x - x_goal;
    float j1 = v + j0*y;
    float eydt = fast_negexpf(y*dt);

    x = eydt*(j0 + j1*dt) + x_goal;
    v = eydt*(v - j1*y*dt);
}

static inline void simple_spring_damper_implicit(
    vec3& x, 
    vec3& v, 
    const vec3 x_goal, 
    const float halflife, 
    const float dt)
{
    float y = halflife_to_damping(halflife) / 2.0f;	
    vec3 j0 = x - x_goal;
    vec3 j1 = v + j0*y;
    float eydt = fast_negexpf(y*dt);

    x = eydt*(j0 + j1*dt) + x_goal;
    v = eydt*(v - j1*y*dt);
}

static inline void simple_spring_damper_implicit(
    quat& x, 
    vec3& v, 
    const quat x_goal, 
    const float halflife, 
    const float dt)
{
    float y = halflife_to_damping(halflife) / 2.0f;	
	
    vec3 j0 = quat_to_scaled_angle_axis(quat_abs(quat_mul(x, quat_inv(x_goal))));
    vec3 j1 = v + j0*y;
	
    float eydt = fast_negexpf(y*dt);

    x = quat_mul(quat_from_scaled_angle_axis(eydt*(j0 + j1*dt)), x_goal);
    v = eydt*(v - j1*y*dt);
}

//--------------------------------------

static inline void decay_spring_damper_implicit(
    float& x, 
    float& v, 
    const float halflife, 
    const float dt)
{
    float y = halflife_to_damping(halflife) / 2.0f;	
    float j1 = v + x*y;
    float eydt = fast_negexpf(y*dt);

    x = eydt*(x + j1*dt);
    v = eydt*(v - j1*y*dt);
}

static inline void decay_spring_damper_implicit(
    vec3& x, 
    vec3& v, 
    const float halflife, 
    const float dt)
{
    float y = halflife_to_damping(halflife) / 2.0f;	
    vec3 j1 = v + x*y;
    float eydt = fast_negexpf(y*dt);

    x = eydt*(x + j1*dt);
    v = eydt*(v - j1*y*dt);
}

static inline void decay_spring_damper_implicit(
    quat& x, 
    vec3& v, 
    const float halflife, 
    const float dt)
{
    float y = halflife_to_damping(halflife) / 2.0f;	
	
    vec3 j0 = quat_to_scaled_angle_axis(x);
    vec3 j1 = v + j0*y;
	
    float eydt = fast_negexpf(y*dt);

    x = quat_from_scaled_angle_axis(eydt*(j0 + j1*dt));
    v = eydt*(v - j1*y*dt);
}

//--------------------------------------

/*
目标发生改变导致offset数据需要重新计算
off_x [in/out]: 位置offset数据，intertialize使用，一般不会手动修改
off_v [in/out]: 速度offset数据，intertialize使用，一般不会手动修改
src_x [in]: 原来的goat位置
src_v [in]: 原来的goat速度
dst_x [in]: 现在的goat位置
dst_v [in]: 现在的goat速度
*/
static inline void inertialize_transition(
    vec3& off_x, 
	vec3& off_v, 
    const vec3 src_x,
	const vec3 src_v,
    const vec3 dst_x,
	const vec3 dst_v)
{
    off_x = (src_x + off_x) - dst_x;
    off_v = (src_v + off_v) - dst_v;
}

/*
out_x [out]: 返回计算好的位置
out_v [out]: 返回计算好的速度
off_x [in/out]: 更新位置offset
off_v [in/out]: 更新速度offset
in_x [in]: 传入现在goat位置
in_v [in]: 传入现在goat速度
other [in]
*/
static inline void inertialize_update(
    vec3& out_x, 
	vec3& out_v,
    vec3& off_x, 
	vec3& off_v,
    const vec3 in_x, 
	const vec3 in_v,
    const float halflife,
    const float dt)
{
    decay_spring_damper_implicit(off_x, off_v, halflife, dt);
    out_x = in_x + off_x;
    out_v = in_v + off_v;
}

static inline void inertialize_transition(
    quat& off_x, 
	vec3& off_v, 
    const quat src_x,
	const vec3 src_v,
    const quat dst_x,
	const vec3 dst_v)
{
    off_x = quat_abs(quat_mul(quat_mul(off_x, src_x), quat_inv(dst_x)));
    off_v = (off_v + src_v) - dst_v;
}

static inline void inertialize_update(
    quat& out_x, 
	vec3& out_v,
    quat& off_x, 
	vec3& off_v,
    const quat in_x, 
	const vec3 in_v,
    const float halflife,
    const float dt)
{
    decay_spring_damper_implicit(off_x, off_v, halflife, dt);
    out_x = quat_mul(off_x, in_x);
    out_v = off_v + in_v;
}
