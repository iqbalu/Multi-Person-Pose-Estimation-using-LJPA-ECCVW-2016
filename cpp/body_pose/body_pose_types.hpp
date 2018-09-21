#ifndef BODY_POSE_TYPES_HH
#define BODY_POSE_TYPES_HH

namespace body_pose{

  // TYPE_BODY_J[Number of Joints]            //  pose style
  enum BodyPoseTypes { FULL_BODY_J13 = 13,   /*   /\
                                                 /| |\ [head, l_sho, r_sho, l_hips, r_hips, l_elb, l_wri, r_elb, r_wri, l_knee, l_ankl, r_knee, r_ankl]
                                                  | |
                                                  \ / */

                        FULL_BODY_J14 = 14, //[head_top, neck, l_sho, r_sho, l_hips, r_hips, l_elb, l_wri, r_elb, r_wri, l_knee, l_ankl, r_knee, r_ankl]
                        FULL_BODY_J15 = 15, //[head_top, neck, l_sho, r_sho, l_hips, r_hips, l_elb, l_wri, r_elb, r_wri, l_knee, l_ankl, r_knee, r_ankl]
                        FULL_BODY_J17 = 17, // [head_top, chin, neck, l_sho', r_sho', abdomen', root', l_hip', r_hip', l_elb', l_wri', r_elb', r_wri' l_knee', l_ankl', r_knee', r_ankle]
                        FULL_BODY_J13_TEMPORAL = 39,
                        UPPER_BODY_J7 = 7,
                        UPPER_BODY_J9 = 9,
                        UPPER_BODY_J10 = 10,
                        UPPER_HALF_BODY_J5 = 5,
                        FC_FULL_BODY_J14 = 196
                      };
}

#endif
