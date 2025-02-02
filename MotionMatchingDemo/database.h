#pragma once

#include "common.h"
#include "vec.h"
#include "quat.h"
#include "array.h"

#include <assert.h>
#include <float.h>
#include <stdio.h>
#include <math.h>

//--------------------------------------

enum
{
    BOUND_SM_SIZE = 16,
    BOUND_LR_SIZE = 64,
};

struct database
{
    /* 
       数据来源于database.bin
       存储骨骼相对于父骨骼的位置信息。切记这里“不要”理解成数据都经过了compute_bone_position_feature的处理，这里的数据都是原生的动画数据帧
       rows表示所有动画帧数量，cols表示骨骼的数量

                  Bone1    Bone2    Bone3   ... BoneCols
       Frame1     {1,2,3}  {1,2,3}  {1,2,3} ... ...
       Frame2     {1,2,3}  {1,2,3}  {1,2,3} ... ...
       Frame3     {1,2,3}  {1,2,3}  {1,2,3} ... ...
       ... 
       FrameRows  {1,2,3}  {1,2,3}  {1,2,3} ... ...
    */
    array2d<vec3> bone_positions;
     
    /*
       数据来源于database.bin
       骨骼速度，刚开始感觉这个速度很好理解，就是骨骼在父空间的速度变化呗，真的是这样吗？自己想想，影响骨骼速度的有哪些？
       1. 父骨骼`位移`产生的`速度`，  即使本骨骼不发生任何位移和旋转，由于父骨骼本身有速度，导致子骨骼在爷空间下也有相同方向的速度
       2. 父骨骼`旋转`产生的`角速度`，即使本骨骼不发生任何位移和旋转，由于父骨骼在旋转，导致子骨骼在爷空间下产生了位移变化
       3. 本骨骼在父空间下位移，父骨骼没有位移以及旋转，本骨骼在父空间下位移产生速度，比如Root, Hips等
       
       bone_velocities 表示的就是本骨骼在父空间下位移产生的速度(上面的3 todo generate_database.py看完后确认下？)，大部分骨骼的值为0，
       因为驱动动画的主要方式还是旋转，当然了，Root和Hips大部分情况下有值。如果这里理解对了，forward_kinematics_velocity 也就好理解了

       bone_velocity = 
            parent_velocity + 
            quat_mul_vec3(parent_rotation, bone_velocities(bone)) + 
            cross(parent_angular_velocity, quat_mul_vec3(parent_rotation, bone_positions(bone)));

        关于最后这个cross使用的是物理公式 线速度v = w*r w为角速度 r为半径矢量

        角速度是矢量。按右手螺旋定则，大拇指方向为ω方向.当质点作逆时针旋转时，ω向上；作顺时针旋转时，ω向下
        设线速度为v，取圆心为原点，设位矢（位置矢量）为r，则
        v=ω×r
    */ 
    array2d<vec3> bone_velocities;

    /* 
       数据来源于database.bin
       存储骨骼相对于父骨骼的旋转信息。
       rows表示所有动画帧数量，cols表示骨骼的数量

                  Bone1      Bone2     Bone3     ... BoneCols
       Frame1     {w,x,y,z}  {w,x,y,z} {w,x,y,z} ... ...
       Frame2     {w,x,y,z}  {w,x,y,z} {w,x,y,z} ... ...
       Frame3     {w,x,y,z}  {w,x,y,z} {w,x,y,z} ... ...
       ... 
       FrameRows  {w,x,y,z}  {w,x,y,z} {w,x,y,z} ... ...
    */
    array2d<quat> bone_rotations;
    
    /*
        数据来源于database.bin
        注意这里是角速度，而不是线速度！
    */
    array2d<vec3> bone_angular_velocities;

    /*
        数据来源于database.bin
        存储父骨骼的Index,RootBone的父骨骼为-1, BoneIndex参考character.h中的enum Bones.
    */
    array1d<int> bone_parents;
    
    /*
        数据来源于database.bin
        printf("range_start:%d,range_end:%d, frames:%d\n", range_starts(r), range_stops(r), features.rows);
        range_start:0,    range_end:281,   frames:53500
        range_start:281,  range_end:562,   frames:53500
        range_start:562,  range_end:13153, frames:53500
        range_start:13153,range_end:25744, frames:53500
        range_start:25744,range_end:39622, frames:53500
        range_start:39622,range_end:53500, frames:53500
    */
    array1d<int> range_starts;
    /*
        数据来源于database.bin
    */
    array1d<int> range_stops;
    
    /*
                Left Foot Position | Right Foot Position | Left Foot Velocity | Right Foot Velocity | Hip Velocity | Trajectory Positions 2D | Trajectory Directions 2D
       Frame1    {标准后的3个float}    {标准后的3个float}    {标准后的3个float}     {标准后的3个float} {标准后的3个float}    {标准后的6个float}        {标准后的6个float} 
       Frame2    {标准后的3个float}    {标准后的3个float}    {标准后的3个float}     {标准后的3个float} {标准后的3个float}    {标准后的6个float}        {标准后的6个float} 
       Frame3    {标准后的3个float}    {标准后的3个float}    {标准后的3个float}     {标准后的3个float} {标准后的3个float}    {标准后的6个float}        {标准后的6个float} 
       ... 
       FrameRows {标准后的3个float}    {标准后的3个float}    {标准后的3个float}     {标准后的3个float} {标准后的3个float}    {标准后的6个float}        {标准后的6个float} 
    */
    array2d<float> features;

    /* 数组长度为Features Number, 内容是所有该Feature的平均值 */
    array1d<float> features_offset;

    /* 数组长度为Features Number, 内容是标准差与weight的差，标准化和逆操作使用 */
    array1d<float> features_scale;
    
    /*
        数据来源于database.bin
    */
    array2d<bool> contact_states;
    
    /* 
        AABB加速查询使用
        n = Frames+Size-1/Size
        存储的内容为管辖范围(比如每16行一组或者每64行一组)内的最小或者最大值

            Left Foot Position | Right Foot Position | Left Foot Velocity | Right Foot Velocity | Hip Velocity | Trajectory Positions 2D | Trajectory Directions 2D
       1         {3个float}           {3个float}           {3个float}            {3个float}         {3个float}          {6个float}                {6个float} 
       2         {3个float}           {3个float}           {3个float}            {3个float}         {3个float}          {6个float}                {6个float} 
       3         {3个float}           {3个float}           {3个float}            {3个float}         {3个float}          {6个float}                {6个float} 
       ...      
       n         {3个float}           {3个float}           {3个float}            {3个float}         {3个float}          {6个float}                {6个float} 
    */
    array2d<float> bound_sm_min;
    array2d<float> bound_sm_max;
    array2d<float> bound_lr_min;
    array2d<float> bound_lr_max;
    
    int nframes() const { return bone_positions.rows; }
    int nbones() const { return bone_positions.cols; }
    int nranges() const { return range_starts.size; }
    int nfeatures() const { return features.cols; }
    int ncontacts() const { return contact_states.cols; }
};

void database_load(database& db, const char* filename)
{
    FILE* f = fopen(filename, "rb");
    assert(f != NULL);
    
    array2d_read(db.bone_positions, f);
    array2d_read(db.bone_velocities, f);
    array2d_read(db.bone_rotations, f);
    array2d_read(db.bone_angular_velocities, f);
    array1d_read(db.bone_parents, f);
    
    array1d_read(db.range_starts, f);
    array1d_read(db.range_stops, f);
    
    array2d_read(db.contact_states, f);
    
    fclose(f);
}

// When we add an offset to a frame in the database there is a chance
// it will go out of the relevant range so here we can clamp it to 
// the last frame of that range.

// todo range_starts range_stops 与 Frames数量的关系？
int database_trajectory_index_clamp(database& db, int frame, int offset)
{
    for (int i = 0; i < db.nranges(); i++)
    {
        if (frame >= db.range_starts(i) && frame < db.range_stops(i))
        {
            return clamp(frame + offset, db.range_starts(i), db.range_stops(i) - 1);
        }
    }
    
    assert(false);
    return -1;
}

//--------------------------------------

/* 使用z-score数据标准化，方便在同一纬度计算Cost来比较
   返回结果符合正态分布，平均值为0，标准差为1
   Param:
        features[in/out]
        features_offset[out]
        features_scale[out]
        others [in]
*/
void normalize_feature(
    slice2d<float> features,
    slice1d<float> features_offset,
    slice1d<float> features_scale,
    const int offset, 
    const int size, 
    const float weight = 1.0f)
{
	// First compute what is essentially the mean 
	// value for each feature dimension
    // 计算平均值
    for (int j = 0; j < size; j++)
    {
        features_offset(offset + j) = 0.0f;    
    }
    
    for (int i = 0; i < features.rows; i++)
    {
        for (int j = 0; j < size; j++)
        {
            // 刚开始以为写的有bug呢，后来一想，a+b+c+d/n = a/n + b/n + c/n + d/n，哈哈, 代码简洁但不一定效率高
            features_offset(offset + j) += features(i, offset + j) / features.rows;
        }
    }
    
	// Now compute the variance of each feature dimension
    // 计算方差
    array1d<float> vars(size);
    vars.zero();
    
    for (int i = 0; i < features.rows; i++)
    {
        for (int j = 0; j < size; j++)
        {
            vars(j) += squaref(features(i, offset + j) - features_offset(offset + j)) / features.rows;
        }
    }
    
	// We compute the overall std of the feature as the average
	// std across all dimensions
    // 计算标准差
    float std = 0.0f;
    for (int j = 0; j < size; j++)
    {
        std += sqrtf(vars(j)) / size;
    }
    
	// Features with no variation can have zero std which is
	// almost always a bug.
    assert(std > 0.0);
    
	// The scale of a feature is just the std divided by the weight
    for (int j = 0; j < size; j++)
    {
        features_scale(offset + j) = std / weight;
    }
    
	// Using the offset and scale we can then normalize the features
    // z-score标准化
    for (int i = 0; i < features.rows; i++)
    {
        for (int j = 0; j < size; j++)
        {
            features(i, offset + j) = (features(i, offset + j) - features_offset(offset + j)) / features_scale(offset + j);
        }
    }
}

/* normalize_feature逆操作，返回源数据
   Param:
        features[in/out]
        features_offset[in]
        features_scale[in]
*/
void denormalize_features(
    slice1d<float> features,
    const slice1d<float> features_offset,
    const slice1d<float> features_scale)
{
    for (int i = 0; i < features.size; i++)
    {
        features(i) = (features(i) * features_scale(i)) + features_offset(i);
    }  
}

//--------------------------------------

/*
   Here I am using a simple recursive version of forward kinematics
   使用简单的递归计算bone相对于root-bone的位置和旋转信息
   Param:
       bone_Position[in/out]
       bone_rotation[in/out]
       others [in]
*/
void forward_kinematics(
    vec3& bone_position,
    quat& bone_rotation,
    const slice1d<vec3> bone_positions,
    const slice1d<quat> bone_rotations,
    const slice1d<int> bone_parents,
    const int bone)
{
    if (bone_parents(bone) != -1)
    {
        vec3 parent_position;
        quat parent_rotation;
        
        forward_kinematics(
            parent_position,
            parent_rotation,
            bone_positions,
            bone_rotations,
            bone_parents,
            bone_parents(bone));
        
        bone_position = quat_mul_vec3(parent_rotation, bone_positions(bone)) + parent_position;
        bone_rotation = quat_mul(parent_rotation, bone_rotations(bone));
    }
    else
    {
        bone_position = bone_positions(bone);
        bone_rotation = bone_rotations(bone); 
    }
}

// Forward kinematics but also compute the velocities
/*
   与forward_kinematics类似，通过递归计算相对于root-bone下bone的位置，旋转，速度和旋转角速度
   理解的话，可以参考上面关于 bone_velocities的解释
   
   Param:
        bone_position [in/out]
        bone_velocity [in/out]
        bone_rotation [in/out]
        bone_angular_velocity [in/out]
        others [in]
*/
void forward_kinematics_velocity(
    vec3& bone_position,
    vec3& bone_velocity,
    quat& bone_rotation,
    vec3& bone_angular_velocity,
    const slice1d<vec3> bone_positions,
    const slice1d<vec3> bone_velocities,
    const slice1d<quat> bone_rotations,
    const slice1d<vec3> bone_angular_velocities,
    const slice1d<int> bone_parents,
    const int bone)
{
	//
    if (bone_parents(bone) != -1)
    {
        vec3 parent_position;
        vec3 parent_velocity;
        quat parent_rotation;
        vec3 parent_angular_velocity;
        
        forward_kinematics_velocity(
            parent_position,
            parent_velocity,
            parent_rotation,
            parent_angular_velocity,
            bone_positions,
            bone_velocities,
            bone_rotations,
            bone_angular_velocities,
            bone_parents,
            bone_parents(bone));
        
        bone_position = quat_mul_vec3(parent_rotation, bone_positions(bone)) + parent_position;
        bone_velocity = 
            parent_velocity + 
            quat_mul_vec3(parent_rotation, bone_velocities(bone)) + 
            cross(parent_angular_velocity, quat_mul_vec3(parent_rotation, bone_positions(bone)));
        bone_rotation = quat_mul(parent_rotation, bone_rotations(bone));
        bone_angular_velocity = quat_mul_vec3(parent_rotation, bone_angular_velocities(bone) + parent_angular_velocity);
    }
    else
    {
        bone_position = bone_positions(bone);
        bone_velocity = bone_velocities(bone);
        bone_rotation = bone_rotations(bone);
        bone_angular_velocity = bone_angular_velocities(bone); 
    }
}

// Compute forward kinematics for all joints
void forward_kinematics_full(
    slice1d<vec3> global_bone_positions,
    slice1d<quat> global_bone_rotations,
    const slice1d<vec3> local_bone_positions,
    const slice1d<quat> local_bone_rotations,
    const slice1d<int> bone_parents)
{
    for (int i = 0; i < bone_parents.size; i++)
    {
		// Assumes bones are always sorted from root onwards
        assert(bone_parents(i) < i);
        
        if (bone_parents(i) == -1)
        {
            global_bone_positions(i) = local_bone_positions(i);
            global_bone_rotations(i) = local_bone_rotations(i);
        }
        else
        {
            vec3 parent_position = global_bone_positions(bone_parents(i));
            quat parent_rotation = global_bone_rotations(bone_parents(i));
            global_bone_positions(i) = quat_mul_vec3(parent_rotation, local_bone_positions(i)) + parent_position;
            global_bone_rotations(i) = quat_mul(parent_rotation, local_bone_rotations(i));
        }
    }
}

// Compute forward kinematics of just some joints using a
// mask to indicate which joints are already computed
void forward_kinematics_partial(
    slice1d<vec3> global_bone_positions,
    slice1d<quat> global_bone_rotations,
    slice1d<bool> global_bone_computed,
    const slice1d<vec3> local_bone_positions,
    const slice1d<quat> local_bone_rotations,
    const slice1d<int> bone_parents,
    int bone)
{
    if (bone_parents(bone) == -1)
    {
        global_bone_positions(bone) = local_bone_positions(bone);
        global_bone_rotations(bone) = local_bone_rotations(bone);
        global_bone_computed(bone) = true;
        return;
    }
    
    if (!global_bone_computed(bone_parents(bone)))
    {
        forward_kinematics_partial(
            global_bone_positions,
            global_bone_rotations,
            global_bone_computed,
            local_bone_positions,
            local_bone_rotations,
            bone_parents,
            bone_parents(bone));
    }
    
    vec3 parent_position = global_bone_positions(bone_parents(bone));
    quat parent_rotation = global_bone_rotations(bone_parents(bone));
    global_bone_positions(bone) = quat_mul_vec3(parent_rotation, local_bone_positions(bone)) + parent_position;
    global_bone_rotations(bone) = quat_mul(parent_rotation, local_bone_rotations(bone));
    global_bone_computed(bone) = true;
}

// Same but including velocity
void forward_kinematics_velocity_partial(
    slice1d<vec3> global_bone_positions,
    slice1d<vec3> global_bone_velocities,
    slice1d<quat> global_bone_rotations,
    slice1d<vec3> global_bone_angular_velocities,
    slice1d<bool> global_bone_computed,
    const slice1d<vec3> local_bone_positions,
    const slice1d<vec3> local_bone_velocities,
    const slice1d<quat> local_bone_rotations,
    const slice1d<vec3> local_bone_angular_velocities,
    const slice1d<int> bone_parents,
    int bone)
{
    if (bone_parents(bone) == -1)
    {
        global_bone_positions(bone) = local_bone_positions(bone);
        global_bone_velocities(bone) = local_bone_velocities(bone);
        global_bone_rotations(bone) = local_bone_rotations(bone);
        global_bone_angular_velocities(bone) = local_bone_angular_velocities(bone);
        global_bone_computed(bone) = true;
        return;
    }
    
    if (!global_bone_computed(bone_parents(bone)))
    {
        forward_kinematics_velocity_partial(
            global_bone_positions,
            global_bone_velocities,
            global_bone_rotations,
            global_bone_angular_velocities,
            global_bone_computed,
            local_bone_positions,
            local_bone_velocities,
            local_bone_rotations,
            local_bone_angular_velocities,
            bone_parents,
            bone_parents(bone));
    }
    
    vec3 parent_position = global_bone_positions(bone_parents(bone));
    vec3 parent_velocity = global_bone_velocities(bone_parents(bone));
    quat parent_rotation = global_bone_rotations(bone_parents(bone));
    vec3 parent_angular_velocity = global_bone_angular_velocities(bone_parents(bone));
    
    global_bone_positions(bone) = quat_mul_vec3(parent_rotation, local_bone_positions(bone)) + parent_position;
    global_bone_velocities(bone) = 
        parent_velocity + 
        quat_mul_vec3(parent_rotation, local_bone_velocities(bone)) + 
        cross(parent_angular_velocity, quat_mul_vec3(parent_rotation, local_bone_positions(bone)));
    global_bone_rotations(bone) = quat_mul(parent_rotation, local_bone_rotations(bone));
    global_bone_angular_velocities(bone) = quat_mul_vec3(parent_rotation, local_bone_angular_velocities(bone) + parent_angular_velocity);
    global_bone_computed(bone) = true;
}

//--------------------------------------

// Compute a feature for the position of a bone relative to the simulation/root bone
/*
   计算bone相对于root-bone的位置，再进行标准化处理存入db.features中，offset累加，供后面传递使用
   Param:
        db [in/out]
        offset [in/out]
        others [in]
*/
void compute_bone_position_feature(database& db, int& offset, int bone, float weight = 1.0f)
{
    for (int i = 0; i < db.nframes(); i++)
    {
        vec3 bone_position;
        quat bone_rotation;
        
        forward_kinematics(
            bone_position,
            bone_rotation,
            db.bone_positions(i),
            db.bone_rotations(i),
            db.bone_parents,
            bone);
        
        // 计算如果root-bone 的rotations，positions全部重置为0后的bone-position位置
        bone_position = quat_mul_vec3(quat_inv(db.bone_rotations(i, 0)), bone_position - db.bone_positions(i, 0));
        
        db.features(i, offset + 0) = bone_position.x;
        db.features(i, offset + 1) = bone_position.y;
        db.features(i, offset + 2) = bone_position.z;
    }
    
    normalize_feature(db.features, db.features_offset, db.features_scale, offset, 3, weight);
    
    offset += 3;
}

// Similar but for a bone's velocity
/*
   计算bone在root-bone空间下的速度，再进行标准化处理存入db.features中，offset累加，供后面传递使用
   Param:
        db [in/out]
        offset [in/out]
        others [in]
*/
void compute_bone_velocity_feature(database& db, int& offset, int bone, float weight = 1.0f)
{
    for (int i = 0; i < db.nframes(); i++)
    {
        vec3 bone_position;
        vec3 bone_velocity;
        quat bone_rotation;
        vec3 bone_angular_velocity;
        
        forward_kinematics_velocity(
            bone_position,
            bone_velocity,
            bone_rotation,
            bone_angular_velocity,
            db.bone_positions(i),
            db.bone_velocities(i),
            db.bone_rotations(i),
            db.bone_angular_velocities(i),
            db.bone_parents,
            bone);
        
        bone_velocity = quat_mul_vec3(quat_inv(db.bone_rotations(i, 0)), bone_velocity);
        
        db.features(i, offset + 0) = bone_velocity.x;
        db.features(i, offset + 1) = bone_velocity.y;
        db.features(i, offset + 2) = bone_velocity.z;
    }
    
    normalize_feature(db.features, db.features_offset, db.features_scale, offset, 3, weight);
    
    offset += 3;
}

// Compute the trajectory at 20, 40, and 60 frames in the future
/* 
    计算Future 20 40 60 Frames的root-bone位置信息(相对于当前帧的root-bone)作为Trajectory Position，再进行标准化处理存入db.features中，offset累加，供后面传递使用
    值得值得注意的是，没有保存垂直位置坐标，因为都是平面行走，没有保存的必要 
    Param:
        db [in/out]
        offset [in/out]
        others [in]
*/
void compute_trajectory_position_feature(database& db, int& offset, float weight = 1.0f)
{
    for (int i = 0; i < db.nframes(); i++)
    {
        int t0 = database_trajectory_index_clamp(db, i, 20);
        int t1 = database_trajectory_index_clamp(db, i, 40);
        int t2 = database_trajectory_index_clamp(db, i, 60);
        
        vec3 trajectory_pos0 = quat_mul_vec3(quat_inv(db.bone_rotations(i, 0)), db.bone_positions(t0, 0) - db.bone_positions(i, 0));
        vec3 trajectory_pos1 = quat_mul_vec3(quat_inv(db.bone_rotations(i, 0)), db.bone_positions(t1, 0) - db.bone_positions(i, 0));
        vec3 trajectory_pos2 = quat_mul_vec3(quat_inv(db.bone_rotations(i, 0)), db.bone_positions(t2, 0) - db.bone_positions(i, 0));
        
        db.features(i, offset + 0) = trajectory_pos0.x;
        db.features(i, offset + 1) = trajectory_pos0.z;
        db.features(i, offset + 2) = trajectory_pos1.x;
        db.features(i, offset + 3) = trajectory_pos1.z;
        db.features(i, offset + 4) = trajectory_pos2.x;
        db.features(i, offset + 5) = trajectory_pos2.z;
    }
    
    normalize_feature(db.features, db.features_offset, db.features_scale, offset, 6, weight);
    
    offset += 6;
}

// Same for direction
/* 
    计算Future 20 40 60 Frames的root-bone方向信息(相对于当前帧的root-bone)作为Trajectory Position，再进行标准化处理存入db.features中，offset累加，供后面传递使用
    值得值得注意的是，没有保存垂直位置坐标，因为都是平面行走，没有保存的必要 
    Param:
        db [in/out]
        offset [in/out]
        others [in]
*/
void compute_trajectory_direction_feature(database& db, int& offset, float weight = 1.0f)
{
    for (int i = 0; i < db.nframes(); i++)
    {
        int t0 = database_trajectory_index_clamp(db, i, 20);
        int t1 = database_trajectory_index_clamp(db, i, 40);
        int t2 = database_trajectory_index_clamp(db, i, 60);
        
        vec3 trajectory_dir0 = quat_mul_vec3(quat_inv(db.bone_rotations(i, 0)), quat_mul_vec3(db.bone_rotations(t0, 0), vec3(0, 0, 1)));
        vec3 trajectory_dir1 = quat_mul_vec3(quat_inv(db.bone_rotations(i, 0)), quat_mul_vec3(db.bone_rotations(t1, 0), vec3(0, 0, 1)));
        vec3 trajectory_dir2 = quat_mul_vec3(quat_inv(db.bone_rotations(i, 0)), quat_mul_vec3(db.bone_rotations(t2, 0), vec3(0, 0, 1)));
        
        db.features(i, offset + 0) = trajectory_dir0.x;
        db.features(i, offset + 1) = trajectory_dir0.z;
        db.features(i, offset + 2) = trajectory_dir1.x;
        db.features(i, offset + 3) = trajectory_dir1.z;
        db.features(i, offset + 4) = trajectory_dir2.x;
        db.features(i, offset + 5) = trajectory_dir2.z;
    }

    normalize_feature(db.features, db.features_offset, db.features_scale, offset, 6, weight);

    offset += 6;
}

// Build the Motion Matching search acceleration structure. Here we
// just use axis aligned bounding boxes regularly spaced at BOUND_SM_SIZE
// and BOUND_LR_SIZE frames
/*
Learned Motion Matching paper中关于加速结构的介绍

Rather than a KD-Tree or clustering-based approach we use a simple
axis-aligned bounding-box (AABB) based method to accelerate the
nearest neighbor search. We fit axis-aligned bounding boxes to
groups of 16 and 64 frames consecutively in X (see Fig 15 for a
visual description).
For each AABB we find the distance from the query point to the
nearest point inside the AABB. If this distance is larger than the
smallest distance so far then no point inside the AABB will have
a smaller distance than our current best, and therefore there is no
need to check inside.
We found that axis-aligned bounding boxes had a number of
interesting advantages for this task. Firstly, as we iterate over the
database in order, we have excellent cache performance and avoid
the random access that can occur using structures such as KD-Trees.
Secondly, the squared distance to an AABB can be computed as a
sum of the squared distance along each dimension individually. This
allows for an essential form of early-out in the search, as the accumulated
 distance to an AABB along just a few dimensions will often
quickly exceed the distance to the best match found so far. Finally,
axis-aligned bounding boxes are simple to use and require a minimal
amount of memory overhead. In our experiments we found two levels 
of hierarchy with the sizes described above enough to greatly
accelerate the search. To accelerate training as well as runtime we
implement this same algorithm in both C++ and Cython[Behnelet al. 2011].
*/
void database_build_bounds(database& db)
{
    int nbound_sm = ((db.nframes() + BOUND_SM_SIZE - 1) / BOUND_SM_SIZE);
    int nbound_lr = ((db.nframes() + BOUND_LR_SIZE - 1) / BOUND_LR_SIZE);
    
    db.bound_sm_min.resize(nbound_sm, db.nfeatures()); 
    db.bound_sm_max.resize(nbound_sm, db.nfeatures()); 
    db.bound_lr_min.resize(nbound_lr, db.nfeatures()); 
    db.bound_lr_max.resize(nbound_lr, db.nfeatures()); 
    
    db.bound_sm_min.set(FLT_MAX);
    db.bound_sm_max.set(FLT_MIN);
    db.bound_lr_min.set(FLT_MAX);
    db.bound_lr_max.set(FLT_MIN);
    
    for (int i = 0; i < db.nframes(); i++)
    {
        int i_sm = i / BOUND_SM_SIZE;
        int i_lr = i / BOUND_LR_SIZE;
        
        for (int j = 0; j < db.nfeatures(); j++)
        {
            db.bound_sm_min(i_sm, j) = minf(db.bound_sm_min(i_sm, j), db.features(i, j));
            db.bound_sm_max(i_sm, j) = maxf(db.bound_sm_min(i_sm, j), db.features(i, j));
            db.bound_lr_min(i_lr, j) = minf(db.bound_lr_min(i_lr, j), db.features(i, j));
            db.bound_lr_max(i_lr, j) = maxf(db.bound_lr_min(i_lr, j), db.features(i, j));
        }
    }
}

// Build all motion matching features and acceleration structure
/*
   从database的数据中提取Feature并且标准化处理存入db.features 中，并且构建AABB加速结构
*/
void database_build_matching_features(
    database& db,
    const float feature_weight_foot_position,
    const float feature_weight_foot_velocity,
    const float feature_weight_hip_velocity,
    const float feature_weight_trajectory_positions,
    const float feature_weight_trajectory_directions)
{
    int nfeatures = 
        3 + // Left Foot Position
        3 + // Right Foot Position 
        3 + // Left Foot Velocity
        3 + // Right Foot Velocity
        3 + // Hip Velocity
        6 + // Trajectory Positions 2D
        6 ; // Trajectory Directions 2D
        
    db.features.resize(db.nframes(), nfeatures);
    db.features_offset.resize(nfeatures);
    db.features_scale.resize(nfeatures);
    
    int offset = 0;
    compute_bone_position_feature(db, offset, Bone_LeftFoot, feature_weight_foot_position);
    compute_bone_position_feature(db, offset, Bone_RightFoot, feature_weight_foot_position);
    compute_bone_velocity_feature(db, offset, Bone_LeftFoot, feature_weight_foot_velocity);
    compute_bone_velocity_feature(db, offset, Bone_RightFoot, feature_weight_foot_velocity);
    compute_bone_velocity_feature(db, offset, Bone_Hips, feature_weight_hip_velocity);
    compute_trajectory_position_feature(db, offset, feature_weight_trajectory_positions);
    compute_trajectory_direction_feature(db, offset, feature_weight_trajectory_directions);
    
    assert(offset == nfeatures);
    
    database_build_bounds(db);
}

// Motion Matching search function essentially consists
// of comparing every feature vector in the database, 
// against the query feature vector, first checking the 
// query distance to the axis aligned bounding boxes used 
// for the acceleration structure.
/*
MotionMatching的查询实现部分
best_index [in/out]    : 一般情况下传入当前frame_index即可，假如当前frame已经是末尾帧了，传入-1即可
best_cost [in/out]     : 传入FLT_MAX即可
range_starts [in]      : 动画帧遍历使用，之所以没有直接使用features.rows是因为动画由几个动画文件组成，而并非一个大的动画文件，range_starts和range_stops提供了每个动画文件的范围
range_stops [in]       : 动画帧遍历使用，同上
features [in]          :
features_offset [in]   : 本函数没有用到
features_scale [in]    : 本函数没有用到
bound_sm_min [in]      : 加速结构，可快速剔除不符合条件的帧，加速查询，这里的算法也特别有意思：

curr_cost += squaref(query_normalized(j) - clampf(query_normalized(j), 
                    bound_lr_min(i_lr, j), bound_lr_max(i_lr, j)));

可以看到，query_normalized(j)仅仅和clampf上的自己做了一个比较，如果连这种方式计算出的cost比当前best_cost还大的话，那么bound里面的任何一个feature都比当前的都要大，就没有比较的必要了

bound_sm_max [in]      : 同上
bound_lr_min [in]      : 同上
bound_lr_max [in]      : 同上
query_normalized [in]  : 当前Pose和Trajectory的标准化数据
transition_cost [in]   : Pose发生改变的固定消耗
ignore_range_end [in]  : range末尾忽略的帧数，比如range范围为1-50，该参数20表示只考虑1-30的帧
ignore_surrounding [in]: 忽略附近的帧数，比如当前帧使用的是50，该参数20表示30-70之间的帧都不考虑，这种方式能保证返回的帧肯定不是当前帧
*/
void motion_matching_search(
    int&   _restrict best_index,
    float& _restrict best_cost,
    const slice1d<int> range_starts,
    const slice1d<int> range_stops,
    const slice2d<float> features,
    const slice1d<float> features_offset,
    const slice1d<float> features_scale,
    const slice2d<float> bound_sm_min,
    const slice2d<float> bound_sm_max,
    const slice2d<float> bound_lr_min,
    const slice2d<float> bound_lr_max,
    const slice1d<float> query_normalized,
    const float transition_cost,
    const int ignore_range_end,
    const int ignore_surrounding)
{
    int nfeatures = query_normalized.size;
    int nranges = range_starts.size;
    
    int curr_index = best_index;
    
    // Find cost for current frame
    if (best_index != -1)
    {
        best_cost = 0.0;
        for (int i = 0; i < nfeatures; i++)
        {
            best_cost += squaref(query_normalized(i) - features(best_index, i));
        }
    }
    
    float curr_cost = 0.0f;
    
    // Search rest of database
    for (int r = 0; r < nranges; r++)
    {
        // Exclude end of ranges from search    
        int i = range_starts(r);
        int range_end = range_stops(r) - ignore_range_end;
        
        while (i < range_end)
        {
            // Find index of current and next large box
            int i_lr = i / BOUND_LR_SIZE;
            int i_lr_next = (i_lr + 1) * BOUND_LR_SIZE;
            
			// Find distance to box
            curr_cost = transition_cost;
            for (int j = 0; j < nfeatures; j++)
            {
                curr_cost += squaref(query_normalized(j) - clampf(query_normalized(j), 
                    bound_lr_min(i_lr, j), bound_lr_max(i_lr, j)));
                
                if (curr_cost >= best_cost)
                {
                    break;
                }
            }
            
			// If distance is greater than current best jump to next box
            if (curr_cost >= best_cost)
            {
                i = i_lr_next;
                continue;
            }
            
			// Check against small box
            while (i < i_lr_next && i < range_end)
            {   
                // Find index of current and next small box
                int i_sm = i / BOUND_SM_SIZE;
                int i_sm_next = (i_sm + 1) * BOUND_SM_SIZE;
                
				// Find distance to box
                curr_cost = transition_cost;
                for (int j = 0; j < nfeatures; j++)
                {
                    curr_cost += squaref(query_normalized(j) - clampf(query_normalized(j), 
                        bound_sm_min(i_sm, j), bound_sm_max(i_sm, j)));
                    
                    if (curr_cost >= best_cost)
                    {
                        break;
                    }
                }
                
				// If distance is greater than current best jump to next box
                if (curr_cost >= best_cost)
                {
                    i = i_sm_next;
                    continue;
                }
                
				// Search inside small box
                while (i < i_sm_next && i < range_end)
                {
                    // Skip surrounding frames
                    if (curr_index != - 1 && abs(i - curr_index) < ignore_surrounding)
                    {
                        i++;
                        continue;
                    }
                    
                    // Check against each frame inside small box
                    curr_cost = transition_cost;
                    for (int j = 0; j < nfeatures; j++)
                    {
                        curr_cost += squaref(query_normalized(j) - features(i, j));
                        if (curr_cost >= best_cost)
                        {
                            break;
                        }
                    }
                    
					// If cost is lower than current best then update best
                    if (curr_cost < best_cost)
                    {
                        best_index = i;
                        best_cost = curr_cost;
                    }
                    
                    i++;
                }
            }
        }
    }
}

// Search database
/*
MotionMatching的核心，查询相似帧并且返回
best_index [in/out]    : 一般情况下传入当前frame_index即可，假如当前frame已经是末尾帧了，传入-1即可
best_cost [in/out]     : 传入FLT_MAX即可
db [in]
query [in]             : 当前Pose和Trajectory的未标准化数据
transition_cost [in]   : Pose发生改变的固定消耗
ignore_range_end [in]  : range末尾忽略的帧数，比如range范围为1-50，该参数20表示只考虑1-30的帧
ignore_surrounding [in]: 忽略附近的帧数，比如当前帧使用的是50，该参数20表示30-70之间的帧都不考虑，这种方式能保证返回的帧肯定不是当前帧
*/
void database_search(
    int& best_index, 
    float& best_cost, 
    const database& db, 
    const slice1d<float> query,
    const float transition_cost = 0.0f,
    const int ignore_range_end = 20,
    const int ignore_surrounding = 20)
{
    // Normalize Query
    array1d<float> query_normalized(db.nfeatures());
    for (int i = 0; i < db.nfeatures(); i++)
    {
        query_normalized(i) = (query(i) - db.features_offset(i)) / db.features_scale(i);
    }
    
    // Search
    motion_matching_search(
        best_index, 
        best_cost, 
        db.range_starts,
        db.range_stops,
        db.features,
        db.features_offset,
        db.features_scale,
        db.bound_sm_min,
        db.bound_sm_max,
        db.bound_lr_min,
        db.bound_lr_max,
        query_normalized,
        transition_cost,
        ignore_range_end,
        ignore_surrounding);
}