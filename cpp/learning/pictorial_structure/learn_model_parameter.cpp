/*
 * learn_model_parameter.cpp
 *
 *  Created on: Mar 18, 2013
 *      Author: mdantone
 */

#include <glog/logging.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include "cpp/learning/pictorial_structure/part.hpp"
#include "cpp/learning/pictorial_structure/utils.hpp"
#include "cpp/learning/pictorial_structure/learn_model_parameter.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include "cpp/body_pose/utils.hpp"
#include "cpp/utils/string_utils.hpp"


using namespace std;
namespace learning
{
namespace ps
{

  void _create_body_model_J13_temporal(learning::ps::Model* model, int nNeighbours)
  {
      CHECK(nNeighbours);

      std::vector<std::string> pair_names;
      {
        using namespace boost::assign;
        //pair_names += "head", "shoulder_r", "shoulder_l", "hip_r", "hip_l","elbow_r","hand_r","elbow_l","hand_l","knee_r","feet_r","knee_l","feet_l";
        pair_names += "head", "shoulder_l", "shoulder_r", "hip_l", "hip_r", "elbow_l", "hand_l", "elbow_r", "hand_r", "knee_l", "feet_l", "knee_r", "feet_r";
      }

      int img_count = 2*nNeighbours+1;
      std::vector<learning::ps::Part> parts(13*img_count);

      for(int iIdx = 0; iIdx < img_count; iIdx++){
        for(int pIdx = 0; pIdx < pair_names.size(); pIdx++){
          std::string str;

          if(iIdx == 0){
             str = boost::str(boost::format("%1%") %pair_names[pIdx]);
          }
          else if(iIdx <= floor(img_count/2)){
            str = boost::str(boost::format("%1%_l_%2%") %pair_names[pIdx] %iIdx);
          }
          else{
            str = boost::str(boost::format("%1%_r_%2%") %pair_names[pIdx] %floor(iIdx/2));
          }
          parts[(iIdx*13)+pIdx] = learning::ps::Part((iIdx*13)+pIdx, str);
        }
      }

      std::vector<int> parents;
      get_joint_constalation(parents, body_pose::FULL_BODY_J13_TEMPORAL);
      model->set_parts(parts, parents);

  }

  void _create_body_model_J13(learning::ps::Model* model){
      std::vector<learning::ps::Part> parts(13);
      parts[0] = learning::ps::Part(0,"head");
      parts[1] = learning::ps::Part(1,"shoulder_l");
      parts[2] = learning::ps::Part(2,"shoulder_r");
      parts[3] = learning::ps::Part(3,"hip_l");
      parts[4] = learning::ps::Part(4,"hip_r");
      parts[5] = learning::ps::Part(5,"elbow_l");
      parts[6] = learning::ps::Part(6,"hand_l");
      parts[7] = learning::ps::Part(7,"elbow_r");
      parts[8] = learning::ps::Part(8,"hand_r");
      parts[9] = learning::ps::Part(9,"knee_l");
      parts[10] = learning::ps::Part(10,"feet_l");
      parts[11] = learning::ps::Part(11,"knee_r");
      parts[12] = learning::ps::Part(12,"feet_r");

      std::vector<int> parents;
      get_joint_constalation( parents, body_pose::FULL_BODY_J13);
      model->set_parts(parts, parents);
  }

  void _create_body_model_J14(learning::ps::Model* model){
      std::vector<learning::ps::Part> parts(14);
      parts[0] = learning::ps::Part(0,"head");
      parts[1] = learning::ps::Part(1,"neck");
      parts[2] = learning::ps::Part(2,"shoulder_l");
      parts[3] = learning::ps::Part(3,"shoulder_r");
      parts[4] = learning::ps::Part(4,"hip_l");
      parts[5] = learning::ps::Part(5,"hip_r");
      parts[6] = learning::ps::Part(6,"elbow_l");
      parts[7] = learning::ps::Part(7,"hand_l");
      parts[8] = learning::ps::Part(8,"elbow_r");
      parts[9] = learning::ps::Part(9,"hand_r");
      parts[10] = learning::ps::Part(10,"knee_l");
      parts[11] = learning::ps::Part(11,"feet_l");
      parts[12] = learning::ps::Part(12,"knee_r");
      parts[13] = learning::ps::Part(13,"feet_r");

      std::vector<int> parents;
      get_joint_constalation( parents, body_pose::FULL_BODY_J14);
      model->set_parts(parts, parents);
  }

    void _create_body_model_J10(learning::ps::Model* model){
      std::vector<learning::ps::Part> parts(10);
      parts[0] = learning::ps::Part(0,"head");
      parts[1] = learning::ps::Part(1,"neck");
      parts[2] = learning::ps::Part(2,"shoulder_l");
      parts[3] = learning::ps::Part(3,"shoulder_r");
      parts[4] = learning::ps::Part(4,"hip_l");
      parts[5] = learning::ps::Part(5,"hip_r");
      parts[6] = learning::ps::Part(6,"elbow_l");
      parts[7] = learning::ps::Part(7,"hand_l");
      parts[8] = learning::ps::Part(8,"elbow_r");
      parts[9] = learning::ps::Part(9,"hand_r");

      std::vector<int> parents;
      get_joint_constalation( parents, body_pose::UPPER_BODY_J10);
      model->set_parts(parts, parents);
  }



  void _create_body_model_J15(learning::ps::Model* model){
        std::vector<learning::ps::Part> parts(15);
      parts[0] = learning::ps::Part(0,"head");
      parts[1] = learning::ps::Part(1,"neck");
      parts[2] = learning::ps::Part(2,"shoulder_l");
      parts[3] = learning::ps::Part(3,"shoulder_r");
      parts[4] = learning::ps::Part(4,"hip_l");
      parts[5] = learning::ps::Part(5,"hip_r");
      parts[6] = learning::ps::Part(6,"elbow_l");
      parts[7] = learning::ps::Part(7,"hand_l");
      parts[8] = learning::ps::Part(8,"elbow_r");
      parts[9] = learning::ps::Part(9,"hand_r");
      parts[10] = learning::ps::Part(10,"knee_l");
      parts[11] = learning::ps::Part(11,"feet_l");
      parts[12] = learning::ps::Part(12,"knee_r");
      parts[13] = learning::ps::Part(13,"feet_r");
      parts[14] = learning::ps::Part(14,"torso");

      std::vector<int> parents;
      get_joint_constalation( parents, body_pose::FULL_BODY_J15);
      model->set_parts(parts, parents);
  }

  void _create_body_model_J17(learning::ps::Model* model){
      std::vector<learning::ps::Part> parts(17);
      parts[0] = learning::ps::Part(0,"head");
      parts[1] = learning::ps::Part(1,"chin");
      parts[1] = learning::ps::Part(2,"neck");
      parts[2] = learning::ps::Part(3,"shoulder_l");
      parts[3] = learning::ps::Part(4,"shoulder_r");
      parts[3] = learning::ps::Part(5,"abdomen");
      parts[3] = learning::ps::Part(6,"pelvic");
      parts[4] = learning::ps::Part(7,"hip_l");
      parts[5] = learning::ps::Part(8,"hip_r");
      parts[6] = learning::ps::Part(9,"elbow_l");
      parts[7] = learning::ps::Part(10,"hand_l");
      parts[8] = learning::ps::Part(11,"elbow_r");
      parts[9] = learning::ps::Part(12,"hand_r");
      parts[10] = learning::ps::Part(13,"knee_l");
      parts[11] = learning::ps::Part(14,"feet_l");
      parts[12] = learning::ps::Part(15,"knee_r");
      parts[13] = learning::ps::Part(16,"feet_r");

      std::vector<int> parents;
      get_joint_constalation( parents, body_pose::FULL_BODY_J17);
      model->set_parts(parts, parents);
  }



  void _create_body_model_J9(learning::ps::Model* model){
      std::vector<learning::ps::Part> parts(9);
      parts[0] = learning::ps::Part(0,"head");
      parts[1] = learning::ps::Part(1,"shoulder_l");
      parts[2] = learning::ps::Part(2,"shoulder_r");
      parts[3] = learning::ps::Part(3,"hip_l");
      parts[4] = learning::ps::Part(4,"hip_r");
      parts[5] = learning::ps::Part(5,"elbow_l");
      parts[6] = learning::ps::Part(6,"hand_l");
      parts[7] = learning::ps::Part(7,"elbow_r");
      parts[8] = learning::ps::Part(8,"hand_r");
      std::vector<int> parents;
      get_joint_constalation( parents, body_pose::UPPER_BODY_J9);
      model->set_parts(parts, parents);
  }

  void _create_body_model_J7(learning::ps::Model* model){
      std::vector<learning::ps::Part> parts(7);
      parts[0] = learning::ps::Part(0,"head");
      parts[1] = learning::ps::Part(1,"shoulder_l");
      parts[2] = learning::ps::Part(2,"shoulder_r");
      parts[3] = learning::ps::Part(3,"elbow_l");
      parts[4] = learning::ps::Part(4,"hand_l");
      parts[5] = learning::ps::Part(5,"elbow_r");
      parts[6] = learning::ps::Part(6,"hand_r");
      std::vector<int> parents;
      get_joint_constalation( parents, body_pose::UPPER_BODY_J7);
      model->set_parts(parts, parents);
  }

  void _create_body_model_J5(learning::ps::Model* model){
      std::vector<learning::ps::Part> parts(5);
      parts[0] = learning::ps::Part(0,"head");
      parts[1] = learning::ps::Part(1,"neck");
      parts[2] = learning::ps::Part(2,"shoulder");
      parts[3] = learning::ps::Part(3,"elbow");
      parts[4] = learning::ps::Part(4,"hand");
      std::vector<int> parents;
      get_joint_constalation( parents, body_pose::UPPER_BODY_J7);
      model->set_parts(parts, parents);
  }

  void _create_body_model_FC_J14(learning::ps::Model* model){

      std::vector<string> part_names;
      part_names += "head", "neck", "shoulder_l", "shoulder_r", "hip_l", "hip_r", "elbow_l", "hand_l", "elbow_r", "hand_r", "knee_l", "feet_l", "knee_r", "feet_r";
      std::vector<learning::ps::Part> parts(196);
      for(unsigned int i=0; i<14; i++){
        for(unsigned int j=0; j<14; j++){
          std::string part_name = boost::str(boost::format("%s_%s") %part_names[i] %part_names[j]);
          parts[i*14+j]   = learning::ps::Part(i*14+j, part_name);
        }
      }
      std::vector<int> parents;
      get_joint_constalation(parents, body_pose::FC_FULL_BODY_J14);
      model->set_parts(parts, parents);
  }


  void create_body_model(learning::ps::Model* model, body_pose::BodyPoseTypes pose_type) {
    int i = 0;
    switch(pose_type){
      case body_pose::FULL_BODY_J15:{
        _create_body_model_J15(model);
        break;
      }
      case body_pose::FULL_BODY_J14:{
        _create_body_model_J14(model);
        break;
      }
      case body_pose::FULL_BODY_J13_TEMPORAL:{
        _create_body_model_J13_temporal(model, 1);
        break;
      }
      case body_pose::UPPER_BODY_J9:{
        _create_body_model_J9(model);
        break;
      }
      case body_pose::UPPER_BODY_J7:{
        _create_body_model_J7(model);
        break;
      }
      case body_pose::FULL_BODY_J13:{
        _create_body_model_J13(model);
        break;
      }
      case body_pose::FULL_BODY_J17:{
        _create_body_model_J17(model);
        break;
      }
      case body_pose::UPPER_BODY_J10:{
        _create_body_model_J10(model);
        break;
      }
      case body_pose::FC_FULL_BODY_J14:{
        _create_body_model_FC_J14(model);
        break;
      }

      default:{
        LOG(ERROR)<<"Body model for given pose type is not defined.";
      }
    }
  }

  void _clean(const std::vector<Annotation> annotations_src,
             std::vector<Annotation>& annotations, int max_size,
             int norm_size, body_pose::BodyPoseTypes pose_type){

      vector<int> parents;
    get_joint_constalation(parents, pose_type);

    for(int i=0; i < annotations_src.size(); i++) {
      bool all_parts_ok = true;
      for(int j=0; j  < annotations_src[i].parts.size(); j++) {
        const cv::Point p = annotations_src[i].parts[j];
        if(p.x <= 0 || p.y <= 0 ) {
          all_parts_ok = false;
        }
      }

      if(!all_parts_ok)
        continue;

      for(int i_part=0; i_part < parents.size(); i_part++) {
        int parents_id = parents[i_part];
        const cv::Point& part = annotations_src[i].parts[i_part];
        const cv::Point& parent = annotations_src[i].parts[parents_id];
        cv::Point offset  = part - parent;

        if(offset.x > max_size || offset.y > max_size) {
          all_parts_ok = false;
        }
      }
      if(!all_parts_ok) {
        continue;
      }
      annotations.push_back(annotations_src[i]);
    }

//    if(norm_size > 0 ) {
//      for(int i=0; i < annotations.size(); i++) {
//
//        int size = get_upperbody_size(annotations[i], pose_type);
//        if( abs(size - norm_size) > 1) {
//
//          float scale = static_cast<float> (norm_size) / size;
//
//          for (unsigned int j = 0; j < annotations[i].parts.size(); j++) {
//            annotations[i].parts[j].x *= scale;
//            annotations[i].parts[j].y *= scale;
//          }
//
//          annotations[i].bbox.x *= scale;
//          annotations[i].bbox.y *= scale;
//          annotations[i].bbox.width *= scale;
//          annotations[i].bbox.height *= scale;
//
//        }
//      }
//    }
  }

  void clean(const std::vector<Annotation> annotations_src,
      std::vector<Annotation>& annotations, int max_size,
      int norm_size, body_pose::BodyPoseTypes pose_type) {

    if(pose_type == body_pose::FULL_BODY_J13 || pose_type == body_pose::FULL_BODY_J14
        || pose_type == body_pose::UPPER_BODY_J9 || pose_type == body_pose::UPPER_BODY_J7 || body_pose::FULL_BODY_J17){
      _clean(annotations_src, annotations, max_size, norm_size, pose_type);
    }
    else{
      LOG(ERROR)<<"Method for cleaning the annotations is not defined";
    }
  }


  void get_body_model(std::string index_path, Model& model,
      body_pose::BodyPoseTypes pose_type, std::vector<JointParameter> params,
      int norm_size) {
    vector<Annotation> annotations;
    CHECK(load_annotations(annotations, index_path));
    CHECK(get_body_model(annotations, model, pose_type, params, norm_size));
  }

  bool get_body_model(const std::vector<Annotation>& annotations, Model& model,
      body_pose::BodyPoseTypes pose_type, std::vector<JointParameter> params, int norm_size) {

    std::vector<double> weights;

    std::vector<Annotation> ann_cleaned;

    create_body_model(&model, pose_type);
    //clean(annotations, ann_cleaned, 110, norm_size, pose_type);
    ann_cleaned = annotations;

    if(params.size() == 0 ){
      params.resize(model.get_num_parts());
    }else{
      CHECK_EQ(params.size(), model.get_num_parts());
    }

    LOG(INFO)<<"Original Annotations = "<<annotations.size()<<" Cleaned Annotations = "<<ann_cleaned.size();
    // if numbers of samples are lesser than number of rotations
    if(ann_cleaned.size() < params[0].num_rotations){
      return false;
    }

    CHECK_GT(ann_cleaned.size(), 0 );

    std::vector< std::vector<cv::Point> > offsets;
    for(int i=0; i < ann_cleaned.size(); i++) {
      offsets.push_back(ann_cleaned[i].parts);
    }

    if(params[0].use_ann_weights){
      weights.resize(ann_cleaned.size());
      for(int i=0; i<ann_cleaned.size(); i++){
        weights[i] = ann_cleaned[i].weight;
      }
    }

    vector<Displacement> deform_cost;
    if(learning::ps::utils::get_displacement_cost(offsets, model.get_parents(), deform_cost, params, pose_type, weights)) {
      model.set_deformation_costs(deform_cost);
      return true;
    }else{
      return false;
    }
  }

  void get_body_model_mixtures(const std::vector<Annotation>& annotations,
                vector<Model>& models,
                body_pose::BodyPoseTypes pose_type,
                std::vector<JointParameter> params,
                int norm_size) {

    // iterate over all annotations and check the cluster_id
    vector<vector<Annotation> > clustered_annotations;
    for(int i=0; i < annotations.size(); i++) {
      const Annotation& ann = annotations[i];
      if(ann.cluster_id >= clustered_annotations.size()) {
        clustered_annotations.resize(ann.cluster_id +1 );
      }
      clustered_annotations[ann.cluster_id].push_back(ann);
    }

    // the size of clustered_annotations represents the number of mixture models.
    // for each mixture model, we calculate the binary potentials
    int n_mixtures = clustered_annotations.size();
    for(int i=0; i < n_mixtures; i++) {

      Model m;
      if(get_body_model(clustered_annotations[i], m, pose_type, params, norm_size) ) {
        m.set_weight( static_cast<float>(clustered_annotations[i].size()) / annotations.size() );
        models.push_back(m);
      }
    }
  }

  void get_body_model_mixtures(std::string index_path,
                     body_pose::clustering::ClusterMethod method,
                     int n_clusters, std::string cluster_path,
                     std::vector<Model>& models,
                     body_pose::BodyPoseTypes pose_type,
                     std::vector<JointParameter> params,
                     int norm_size,
                     bool use_flipped_anns) {

      get_body_model_mixtures(index_path, method, n_clusters, cluster_path,
                              models, pose_type, std::vector<int>(), params, norm_size, use_flipped_anns);

  }


  void get_body_model_mixtures(std::string index_path,
                     body_pose::clustering::ClusterMethod method,
                     int n_clusters, std::string cluster_path,
                     std::vector<Model>& models,
                     body_pose::BodyPoseTypes pose_type,
                     std::vector<int> cluster_ids,
                     std::vector<JointParameter> params,
                     int norm_size,
                     bool use_flipped_anns) {

    vector<Annotation> org_annotations, annotations;

    CHECK(load_annotations(org_annotations, index_path));

    if(cluster_ids.size()){
      keep_ann_of_cluster_ids(org_annotations, annotations, cluster_ids, false);
      LOG(INFO)<<"Annotations for given cluster: "<<annotations.size();
    }
    else{
      annotations = org_annotations;
    }

    // use flipped annotations
    if(use_flipped_anns){
      add_flipped_annotations(annotations, pose_type);
    }

    LOG(INFO)<<"Annotations selected based on class label = "<<annotations.size();
    CHECK(annotations.size());

    if(n_clusters > 1) {
        vector<cv::Mat> clusters;
        CHECK(body_pose::clustering::load_clusters(cluster_path, n_clusters, method, clusters, pose_type) );

        vector<Annotation> clustered_ann;
        std::vector<int> part_indices;

        body_pose::clustering::assigne_to_clusters(annotations, part_indices, clusters[0], method, clustered_ann, pose_type);
        get_body_model_mixtures( clustered_ann, models, pose_type, params, norm_size);

    }else{
      for(int i=0 ; i < annotations.size(); i++) {
        annotations[i].cluster_id = 0;
      }
      get_body_model_mixtures( annotations, models, pose_type, params, norm_size);
    }
  }

  // creates body model when multi-person annotation file has to be used
  void get_body_model_mixtures_ma(std::string index_path,
                     body_pose::clustering::ClusterMethod method,
                     int n_clusters, std::string cluster_path,
                     std::vector<Model>& models,
                     body_pose::BodyPoseTypes pose_type,
                     std::vector<int> cluster_ids,
                     std::vector<JointParameter> params,
                     int norm_size,
                     bool use_flipped_anns) {

    vector<MultiAnnotation> m_annotations;
    CHECK(load_multi_annotations(m_annotations, index_path));

    vector<Annotation> org_annotations, annotations;
    for(unsigned int i=0; i<m_annotations.size(); i++){
      for(unsigned int n=0; n<m_annotations[i].persons.size(); n++){
        Annotation ann = m_annotations[i].persons[n];
        rescale_ann(ann, 1.0/ann.scale);
        org_annotations.push_back(ann);
      }
    }

    if(cluster_ids.size()){
      keep_ann_of_cluster_ids(org_annotations, annotations, cluster_ids, false);
      LOG(INFO)<<"Annotations for given cluster: "<<annotations.size();
    }
    else{
      annotations = org_annotations;
    }

    // use flipped annotations
    if(use_flipped_anns){
      add_flipped_annotations(annotations, pose_type);
    }

    LOG(INFO)<<"Annotations selected based on class label = "<<annotations.size();
    CHECK(annotations.size());

    if(n_clusters > 1) {
        vector<cv::Mat> clusters;
        CHECK(body_pose::clustering::load_clusters(cluster_path, n_clusters, method, clusters, pose_type) );

        vector<Annotation> clustered_ann;
        std::vector<int> part_indices;

        body_pose::clustering::assigne_to_clusters(annotations, part_indices, clusters[0], method, clustered_ann, pose_type);
        get_body_model_mixtures( clustered_ann, models, pose_type, params, norm_size);

    }else{
      for(int i=0 ; i < annotations.size(); i++) {
        annotations[i].cluster_id = 0;
      }
      get_body_model_mixtures( annotations, models, pose_type, params, norm_size);
    }
  }



  void get_actions_based_body_model(std::string index_path,
                     Model& model,
                     std::vector<JointParameter> params,
                     int norm_size,
                     body_pose::BodyPoseTypes pose_type,
                     std::vector<double> actions_probs,
                     std::vector<int> actionLabelIdx) {

        for(unsigned int i=0; i<params.size(); i++){
          params[i].use_ann_weights = true;
        }

        vector<Annotation> annotations;
        CHECK(load_annotations(annotations, index_path));
        for(int i=0 ; i < annotations.size(); i++) {
          double p = actions_probs[actionLabelIdx[annotations[i].cluster_id]];
          annotations[i].weight = p > 0 ? p:0;
        }

        int nAnn = annotations.size();
        for(int i=0; i < nAnn; i++){
          Annotation ann = annotations[i];
          flip_parts(ann, pose_type);
          annotations.push_back(ann);
        }
        CHECK(get_body_model(annotations, model, pose_type, params, norm_size));
  }

} // ps
} // lerning
