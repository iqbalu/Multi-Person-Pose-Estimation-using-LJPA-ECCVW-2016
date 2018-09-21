
#include "cpp/learning/SPLPI/learn_model_parameter.hpp"
#include "cpp/learning/SPLPI/utils.hpp"
#include "cpp/learning/logistic_regression/LogisticRegression.hpp"
#include "cpp/body_pose/utils.hpp"

#include "boost/random.hpp"
#include "boost/generator_iterator.hpp"
#include <boost/math/special_functions/factorials.hpp>
#include <boost/progress.hpp>
#include "cpp/learning/pictorial_structure/learn_model_parameter.hpp"
#include "cpp/learning/pictorial_structure/utils.hpp"
#include "cpp/vision/geometry_utils.hpp"
#include "cpp/utils/libsvm/libsvm.hpp"


using namespace std;
using namespace cv;
using boost::shared_ptr;

namespace learning
{

namespace splpi
{



bool learn_param_for_same_class_binaries_mp(vector<MultiAnnotation>& annotations,
                              SPLPI& splp_model,
                              int patch_size,
                              body_pose::BodyPoseTypes pose_type,
                              string cache)

{
  vector<splpi::BinaryModel> models;
  models.resize((int)pose_type);
  boost::mt19937 rng;

  int N = 3; // locations from each sample

  int total_persons = 0;
  for(unsigned int aIdx=0; aIdx<annotations.size(); aIdx++){
    total_persons += annotations[aIdx].persons.size();
  }

  CHECK(total_persons);

  vector<Mat> pos_samples(static_cast<int>(pose_type));
  vector<Mat> neg_samples(static_cast<int>(pose_type));

  // Rough estimate of number of samples to allocate memory in advance
  int num_pos = total_persons * (boost::math::factorial<double>(N)/(2*boost::math::factorial<double>(N-2)));
  int num_neg = total_persons * N * N * (static_cast<int>(pose_type)-1);

  for(unsigned int pIdx=0; pIdx<static_cast<unsigned int>(pose_type); pIdx++){
    pos_samples[pIdx] = Mat::zeros(num_pos, 18, CV_32F);
    neg_samples[pIdx] = Mat::zeros(num_neg, 18, CV_32F);
  }

  vector<int> pos_count(static_cast<int>(pose_type),0);
  vector<int> neg_count(static_cast<int>(pose_type),0);

  for(unsigned int aIdx=0; aIdx<annotations.size(); aIdx++){

    MultiAnnotation ann = annotations[aIdx];
    vector<cv::Point> within_image_parts;
    within_image_parts.reserve(ann.persons.size()*(int)pose_type);


    for(unsigned int nIdx=0; nIdx<annotations[aIdx].persons.size(); nIdx++){
      rescale_ann(ann.persons[nIdx], 1.0/ann.persons[0].scale);
      within_image_parts.insert(within_image_parts.end(), ann.persons[nIdx].parts.begin(), ann.persons[nIdx].parts.end());
    }

    boost::normal_distribution<> nd(0.0, 7.5);
    boost::variate_generator<boost::mt19937&, boost::normal_distribution<> > rand_gauss(rng, nd);
    rng.seed(aIdx);
    srand(aIdx);

    for(unsigned int p=0; p<within_image_parts.size(); p++){
      int part_id = p % ((int)pose_type);
      cv::Point loc = within_image_parts[p];
      if(loc.x < 0 || loc.y < 0){
         continue;
      }

      // get some samples around the location to be used as +ve samples
      vector<cv::Point> pos_locs;
      for(unsigned int lIdx=0; lIdx<N; lIdx++){
        cv::Point l = cv::Point(loc.x+rand_gauss(), loc.y+rand_gauss());
        if(lIdx == 0){
          l = loc;
        }
        if(l.x < 0 || l.y < 0){
         lIdx--;
         continue;
        }
//        LOG(INFO)<<l.x<<"\t"<<l.y;
        pos_locs.push_back(l);
      }

      // extract features for +ve locations
      for(unsigned int lIdx=0; lIdx<pos_locs.size()-1; lIdx++){
        cv::Point l1 = pos_locs[lIdx];
        for(unsigned int llIdx=lIdx+1; llIdx<pos_locs.size(); llIdx++){
          cv::Point l2 = pos_locs[llIdx];
          cv::Mat feat = compute_spatial_relation_feature(l1, l2, patch_size);
          feat.copyTo(pos_samples[part_id].row(pos_count[part_id]));
          pos_count[part_id]++;
        }
      }

      vector<cv::Point> neg_locs;
      for(unsigned int pp=0; pp<within_image_parts.size(); pp=pp+rand()%10){
        if(p != pp){
          Point n_loc = within_image_parts[pp];
          if(n_loc.x < 0 || n_loc.y < 0){
            continue;
          }
          neg_locs.push_back(n_loc);
        }
      }

      // extract -ve features
      for(unsigned int lIdx=0; lIdx<pos_locs.size(); lIdx++){
        cv::Point l1 = pos_locs[lIdx];
        for(unsigned int llIdx=0; llIdx<neg_locs.size(); llIdx++){
          cv::Point l2 = neg_locs[llIdx];
          cv::Mat feat = compute_spatial_relation_feature(l1, l2, patch_size);
          feat.copyTo(neg_samples[part_id].row(neg_count[part_id]));
          neg_count[part_id]++;
        }
      }
    }
  }

  for(unsigned int pIdx=0; pIdx<int(pose_type); pIdx++){
    pos_samples[pIdx] = pos_samples[pIdx].rowRange(0, pos_count[pIdx]);
    neg_samples[pIdx] = neg_samples[pIdx].rowRange(0, neg_count[pIdx]);
  }

  // FIXME: Can be made parallel for better performance
  for(unsigned int pIdx=0; pIdx<static_cast<unsigned int>(pose_type); pIdx++){
    cout<<"Learning same-class binary params for Class: "<<pIdx<<endl;
    Mat p_sample = pos_samples[pIdx];
    Mat p_labels = Mat::ones(p_sample.rows, 1, CV_32F);

    Mat n_sample = neg_samples[pIdx];
    n_sample = shuffleRows(n_sample);
    Mat n_labels = Mat::ones(n_sample.rows, 1, CV_32F)*-1;

    Mat data, labels;
    vconcat(p_sample, n_sample.rowRange(0, p_sample.rows), data);
    vconcat(p_labels, n_labels.rowRange(0, p_sample.rows), labels);

    // combine data and labels to shuffle them together
    Mat comb_data_label;
    hconcat(data, labels, comb_data_label);
    comb_data_label = shuffleRows(comb_data_label);

    // get the values and train
    data = comb_data_label.colRange(0, comb_data_label.cols-1);
    labels = comb_data_label.col(comb_data_label.cols-1);

    Mat norm_data, mean, std;
    normalize_data_range(data, norm_data, mean, std);

    string path = boost::str(boost::format("%s/%02d.txt") %cache %pIdx);
    models[pIdx].path = path;

    if(boost::filesystem::exists(path.c_str())){
      continue;
    }

    SvmProblem problem;
    SvmParam param = ::utils::libsvm::get_default_svm_param();
    param.eps = 1e-3;
    param.probability = 1; /* do probability estimates */
    param.shrinking = 1;
    param.cache_size = 2000;


    for(unsigned int r=0; r<norm_data.rows; r++){
      CHECK(problem.push_problem<float>(norm_data.row(r), (double)labels.at<float>(r, 0)));
    }

    models[pIdx].model->train(problem, param);
    models[pIdx].mean = mean;
    models[pIdx].std = std;
    models[pIdx].model->write_as_text(path.c_str());

    if(true){
      std::vector<double> values;
      int true_positive = 0;
      for(unsigned int r=0; r<norm_data.rows; r++){
        int class_label = models[pIdx].model->predict_probability<float>(norm_data.row(r), values);
        if(class_label == (int)labels.at<float>(r, 0)){
          true_positive++;
        }
      }
      LOG(INFO)<<"Accuracy = "<<true_positive/(float)norm_data.rows*100;
    }
  }
  CHECK(splp_model.set_same_class_binary_models(models))<<
            "Failed during setting SPLP same class binary models";
  return true;
}


void visualize_offsets(Mat& pos_feat, Mat& neg_feat, int pIdx, int ppIdx)
{
  int size = 500;
  cv::Mat img1(size,size,CV_8UC3, cv::Scalar(255,255,255));
  cv::Mat img2(size,size,CV_8UC3, cv::Scalar(255,255,255));

  cv::Point center(size/2,size/2);

  for(unsigned int i=0; i<pos_feat.rows; i++){
    cv::Point_<float> pt(pos_feat.at<float>(i,0)+250,pos_feat.at<float>(i,1)+250);
    cv::circle(img1, pt, 2, cv::Scalar(0,255, 0),-1);
  }

  for(unsigned int i=0; i<neg_feat.rows; i++){
    cv::Point_<float> pt(neg_feat.at<float>(i,0)+250,neg_feat.at<float>(i,1)+250);
    cv::circle(img2, pt, 2, cv::Scalar(0,0, 255),-1);
  }

  cv::Mat mean = (img1 + img2)/2;
  cv::circle(mean, center, 2, cv::Scalar(0,0, 0),-1);
  string name = boost::str(boost::format("%d_%d.png") %pIdx %ppIdx);
  //imshow(name.c_str(), mean);
  imwrite(name, mean);
//  waitKey(100);
}

void train_diff_class_svm_mt(vector<vector<Mat> >& pos_feat_per_pair,
                              vector<vector<Mat> >& neg_feat_per_pair,
                              splpi::BinaryModel* m,
                              int pIdx, int ppIdx,
                              string cache)
{
  string path = boost::str(boost::format("%s/%02d_%02d.txt") %cache %pIdx %ppIdx);
  LOG(INFO)<<path;
  m->model.reset(new SVM);

  cv::Mat pos_feat = pos_feat_per_pair[pIdx][ppIdx];
  cv::Mat neg_feat = neg_feat_per_pair[pIdx][ppIdx];

  // take features from other parts as negatives as well.
  for(unsigned int p=0; p<pos_feat_per_pair.size(); p++){
    for(unsigned int pp=0; pp<pos_feat_per_pair.size(); pp++){
      if((p == pIdx && pp == ppIdx) || p == pp){
        continue;
      }

      if(neg_feat.rows == 0){
        neg_feat = pos_feat_per_pair[p][pp].clone();
      }
      else{
        vconcat(neg_feat, pos_feat_per_pair[p][pp], neg_feat);
      }
    }
  }
  neg_feat = shuffleRows(neg_feat);
  cv::Mat selected_neg_feat = neg_feat.rowRange(0, std::min(neg_feat.rows, pos_feat.rows));

  visualize_offsets(pos_feat, selected_neg_feat, pIdx, ppIdx);

  cv::Mat pos_label = cv::Mat::ones(pos_feat.rows, 1, CV_32F);
  cv::Mat neg_label = cv::Mat::ones(selected_neg_feat.rows, 1, CV_32F)*-1;

  cv::Mat data, labels;
  vconcat(pos_feat, selected_neg_feat, data);
  vconcat(pos_label, neg_label, labels);

  LOG(INFO)<<path<<"\t Pos. Samples = "<<pos_feat.rows<<"\t Neg. Samples = "<<neg_feat.rows;

  // combine data and labels to shuffle them together
  Mat comb_data_label;
  hconcat(data, labels, comb_data_label);
  comb_data_label = shuffleRows(comb_data_label);

  // get the values back
  data = comb_data_label.colRange(0, comb_data_label.cols-1);
  labels = comb_data_label.col(comb_data_label.cols-1);

  Mat norm_data, mean, std;
  normalize_data_range(data, norm_data, mean, std);
  m->mean = mean;
  m->std = std;
  m->path = path;

  SvmProblem problem;
  SvmParam param = ::utils::libsvm::get_default_svm_param();
  param.eps = 1e-4;
  param.probability = 1; /* do probability estimates */
  param.C = 2.5;
  param.gamma = 0.5;
  param.cache_size = 16000;

  for(unsigned int r=0; r<norm_data.rows; r++){
    CHECK(problem.push_problem<float>(norm_data.row(r), (double)labels.at<float>(r, 0)));
  }

  bool hard_negative_mining = false;
  if(boost::filesystem::exists(path.c_str())) {
    m->model->read_from_text(path.c_str());
  }else{
    m->model->train(problem, param);
    hard_negative_mining = true;
  }

  int num_iter = 1;
  if(hard_negative_mining){
    param.nr_weight=2;
    param.weight_label = new int[2];
    param.weight = new double[2];
    param.weight_label[0]=-1;
    param.weight_label[1]=+1;
    double Cn = 1;
    double Cp = 20;
    param.weight[0]=Cn;
    param.weight[1]=Cp;

    for(int i=0; i<num_iter; i++){
      int true_pos = 0;
      Mat hard_negs;
      for(unsigned int n=0; n<neg_feat.rows; n++){
        Mat feat = neg_feat.row(n);
        normalize_data(feat, feat, mean, std);
        vector<double> probs;
        float resp = m->model->predict_probability<float>(feat, probs);
        if(resp==-1){
          true_pos++;
        }
        else{
          if(hard_negs.rows == 0){
            hard_negs = feat;
          }else{
            vconcat(hard_negs, feat, hard_negs);
          }

        }
      }
      LOG(INFO)<<path<<"\tIter: "<<i<<"\tAccuracy on all negatvies = "<<true_pos/(float)neg_feat.rows*100;
      LOG(INFO)<<path<<"\tHard negative counts: "<<hard_negs.rows;
      for(unsigned int r=0; r<hard_negs.rows; r++){
        CHECK(problem.push_problem<float>(hard_negs.row(r), -1));
      }
      m->model.reset(new SVM);
      m->model->train(problem, param);
    }
    m->model->write_as_text(path.c_str());
  }

  if(true){
    int true_pos = 0;
    int cor_pos = 0, cor_neg = 0;
    for(unsigned int n=0; n<norm_data.rows; n++){
     vector<double> probs;
     float resp = m->model->predict_probability<float>(norm_data.row(n), probs);
     if(resp==labels.at<float>(n,0)){
      true_pos++;
      if(resp == -1){
        cor_neg++;
      }
      else{
        cor_pos++;
      }
     }
    }
    LOG(INFO)<<path<<"\tAccuracy = "<<true_pos/(float)norm_data.rows*100 << "\tPos:\t" << cor_pos/(float)norm_data.rows*100 << "\tNeg:\t" << cor_neg/(float)norm_data.rows*100;
  }
  neg_feat_per_pair[pIdx][ppIdx] = cv::Mat();
}

bool learn_param_for_diff_class_binaries_mp(vector<MultiAnnotation>& annotations,
                                         vector<vector<Mat> >& appearance_feats,
                                         SPLPI& splp_model,
                                         body_pose::BodyPoseTypes pose_type,
                                         string cache,
                                         int num_threads)
{
  vector<vector<vector<cv::Point> > > offsets_per_pair((int)pose_type);
  vector<vector<cv::Mat> > pos_feat_per_pair((int)pose_type);
  vector<vector<cv::Mat> > neg_feat_per_pair((int)pose_type);
  vector<vector<int> > p_count((int)pose_type);
  vector<vector<int> > n_count((int)pose_type);

  for(unsigned int pIdx=0; pIdx<(int)pose_type; pIdx++){
    offsets_per_pair[pIdx].resize((int)pose_type);
    pos_feat_per_pair[pIdx].resize((int)pose_type);
    neg_feat_per_pair[pIdx].resize((int)pose_type);
    p_count[pIdx].resize((int)pose_type);
    n_count[pIdx].resize((int)pose_type);
  }

  int total_persons = 0;
  for(unsigned int aIdx=0; aIdx<annotations.size(); ++aIdx){
//    total_persons += annotations[aIdx].person_groups.size();
    total_persons += annotations[aIdx].persons.size();
  }
  LOG(INFO)<<"Total persons = "<<total_persons;

  cv::Mat test;
  extract_pairwise_feat_from_offset(cv::Point(-1,-1), test);
  int num_feat = appearance_feats[0][0].row(0).cols - 1; // last value is the check flag
  hconcat(test, appearance_feats[0][0].row(0).colRange(0,num_feat), test);
  hconcat(test, appearance_feats[0][0].row(0).colRange(0,num_feat), test);

  for(unsigned int pIdx=0; pIdx<(int)pose_type; pIdx++){
    for(unsigned int ppIdx=0; ppIdx<(int)pose_type; ppIdx++){
      if(pIdx == ppIdx){ continue; }
      pos_feat_per_pair[pIdx][ppIdx] = Mat::zeros(total_persons, test.cols, CV_32F);
      neg_feat_per_pair[pIdx][ppIdx] = Mat::zeros(total_persons*50, test.cols, CV_32F);
    }
  }

  LOG(INFO)<<"Computing pairwise offsets.";

  boost::progress_display show_progress(annotations.size());
  int wrong_count = 0;
  for(unsigned int aIdx=0; aIdx<annotations.size(); ++aIdx, ++show_progress){

    MultiAnnotation ann = annotations[aIdx];
    CHECK_EQ(appearance_feats[aIdx].size(), ann.persons.size());

    for(unsigned int nIdx=0; nIdx<ann.persons.size(); nIdx++){
      int person_id = nIdx;

      float scale = ann.persons[person_id].scale;
      rescale_ann(ann.persons[person_id], scale);

      for(unsigned int pIdx=0; pIdx<(int)pose_type; pIdx++){
        for(unsigned int ppIdx=0; ppIdx<(int)pose_type; ppIdx++){

          if(pIdx == ppIdx){
            continue;
          }

          cv::Point p1 = ann.persons[person_id].parts[pIdx];
          cv::Point p2 = ann.persons[person_id].parts[ppIdx];

          if(!is_valid(p1) || !is_valid(p2)){
            continue;
          }

          cv::Point offset = p2 - p1;
          offsets_per_pair[pIdx][ppIdx].push_back(offset);

          cv::Mat feat;
          extract_pairwise_feat_from_offset(offset, feat);
          hconcat(feat, appearance_feats[aIdx][nIdx].row(pIdx).colRange(0,num_feat), feat); // nIdx: features of person groups are saved only
          hconcat(feat, appearance_feats[aIdx][nIdx].row(ppIdx).colRange(0,num_feat), feat);

          bool check_p = static_cast<bool>(appearance_feats[aIdx][nIdx].row(pIdx).at<float>(0,num_feat));
          bool check_pp = static_cast<bool>(appearance_feats[aIdx][nIdx].row(ppIdx).at<float>(0,num_feat));

          if(check_p && check_pp){
            CHECK_LT(p_count[pIdx][ppIdx], pos_feat_per_pair[pIdx][ppIdx].rows);
            feat.copyTo(pos_feat_per_pair[pIdx][ppIdx].row(p_count[pIdx][ppIdx]));
            p_count[pIdx][ppIdx]++;
          }
          else{
            CHECK_LT(n_count[pIdx][ppIdx], neg_feat_per_pair[pIdx][ppIdx].rows);
            feat.copyTo(neg_feat_per_pair[pIdx][ppIdx].row(n_count[pIdx][ppIdx]));
            n_count[pIdx][ppIdx]++;
          }

          // gather negatives from other person
/*          for(unsigned int nnIdx=0; nnIdx<ann.person_groups.size(); nnIdx++){
            int other_person_id = ann.person_groups[nnIdx];
            if(person_id == other_person_id){
              continue;
            }

            for(unsigned int p=0; p<(int)pose_type; p++){

              cv::Point pt = ann.persons[other_person_id].parts[p];
              if(!is_valid(pt)){
                continue;
              }

              cv::Point offset = pt - p1;
              cv::Mat feat1;
              extract_pairwise_feat_from_offset(offset, feat1);
              hconcat(feat1, appearance_feats[aIdx][nIdx].row(pIdx).colRange(0,num_feat), feat1);
              hconcat(feat1, appearance_feats[aIdx][nnIdx].row(p).colRange(0,num_feat), feat1);
              CHECK_LT(n_count[pIdx][ppIdx], neg_feat_per_pair[pIdx][ppIdx].rows);
              feat1.copyTo(neg_feat_per_pair[pIdx][ppIdx].row(n_count[pIdx][ppIdx]));
              n_count[pIdx][ppIdx]++;

              offset = pt - p2;
              cv::Mat feat2;
              extract_pairwise_feat_from_offset(offset, feat2);
              hconcat(feat2, appearance_feats[aIdx][nIdx].row(ppIdx).colRange(0,num_feat), feat2);
              hconcat(feat2, appearance_feats[aIdx][nnIdx].row(p).colRange(0,num_feat), feat2);
              CHECK_LT(n_count[pIdx][ppIdx], neg_feat_per_pair[pIdx][ppIdx].rows);
              feat2.copyTo(neg_feat_per_pair[pIdx][ppIdx].row(n_count[pIdx][ppIdx]));
              n_count[pIdx][ppIdx]++;

            }
          }*/
        }
      }
    }
  }

  for(unsigned int pIdx=0; pIdx<(int)pose_type; pIdx++){
    for(unsigned int ppIdx=0; ppIdx<(int)pose_type; ppIdx++){
      if(pIdx == ppIdx){ continue; }
      pos_feat_per_pair[pIdx][ppIdx] = pos_feat_per_pair[pIdx][ppIdx].rowRange(0, p_count[pIdx][ppIdx]);
      neg_feat_per_pair[pIdx][ppIdx] = neg_feat_per_pair[pIdx][ppIdx].rowRange(0, n_count[pIdx][ppIdx]);
    }
  }

  if(num_threads < 1){
    num_threads = ::utils::system::get_available_logical_cpus();
  }

  vector<vector<splpi::BinaryModel> > models((int)pose_type);
  num_threads = 14;
  if(num_threads >= 1){
    for(unsigned int pIdx=0; pIdx<(int)pose_type; pIdx++){
      boost::thread_pool::executor e(num_threads);
      models[pIdx].resize((int)pose_type);
      for(unsigned int ppIdx=0; ppIdx<(int)pose_type; ppIdx++){

        if(pIdx == ppIdx){
          continue;
        }
        e.submit(boost::bind(&train_diff_class_svm_mt, pos_feat_per_pair,  neg_feat_per_pair, &models[pIdx][ppIdx], pIdx, ppIdx, cache));
      }
      e.join_all();
      for(unsigned int ppIdx=0; ppIdx<(int)pose_type; ppIdx++){
          neg_feat_per_pair[pIdx][ppIdx].release();
      }
    }
  }
  else{
    for(unsigned int pIdx=0; pIdx<(int)pose_type; pIdx++){
      models[pIdx].resize((int)pose_type);
      for(unsigned int ppIdx=0; ppIdx<(int)pose_type; ppIdx++){

        if(pIdx == ppIdx){
          continue;
        }

        train_diff_class_svm_mt(pos_feat_per_pair, neg_feat_per_pair, &models[pIdx][ppIdx], pIdx, ppIdx, cache);
      }
    }
  }

  CHECK(splp_model.set_diff_class_binary_models(models))<<
            "Failed during setting SPLP difference class binary models";
  return true;
}

void train_unary_svm_mt(vector<Mat>& pos_feats,
                           vector<Mat>& neg_feats,
                              splpi::BinaryModel* m,
                              int pIdx,
                              string cache)
{
  string path = boost::str(boost::format("%s/%02d.txt") %cache %pIdx);
  LOG(INFO)<<path;
  m->model.reset(new SVM);

  cv::Mat pos_feat = pos_feats[pIdx];
  cv::Mat neg_feat = neg_feats[pIdx];

  // take features from other parts as negatives as well.
  for(unsigned int p=0; p<pos_feats.size(); p++){
    if(p == pIdx){ continue;}

    if(neg_feat.rows == 0){
      neg_feat = pos_feats[p].clone();
    }
    else{
      vconcat(neg_feat, pos_feats[p], neg_feat);
    }
  }

  neg_feat = shuffleRows(neg_feat);
  cv::Mat selected_neg_feat = neg_feat.rowRange(0, std::min(neg_feat.rows, pos_feat.rows));

  cv::Mat pos_label = cv::Mat::ones(pos_feat.rows, 1, CV_32F);
  cv::Mat neg_label = cv::Mat::ones(selected_neg_feat.rows, 1, CV_32F)*-1;

  cv::Mat data, labels;
  vconcat(pos_feat, selected_neg_feat, data);
  vconcat(pos_label, neg_label, labels);

  LOG(INFO)<<path<<"\t Pos. Samples = "<<pos_feat.rows<<"\t Neg. Samples = "<<neg_feat.rows;

  // combine data and labels to shuffle them together
  Mat comb_data_label;
  hconcat(data, labels, comb_data_label);
  comb_data_label = shuffleRows(comb_data_label);

  // get the values back
  data = comb_data_label.colRange(0, comb_data_label.cols-1);
  labels = comb_data_label.col(comb_data_label.cols-1);

  Mat norm_data, mean, std;
  normalize_data_range(data, norm_data, mean, std);
  m->mean = mean;
  m->std = std;
  m->path = path;

  SvmProblem problem;
  SvmParam param = ::utils::libsvm::get_default_svm_param();
  param.eps = 1e-5;
  param.probability = 1; /* do probability estimates */
  param.C = 2.5;
  param.gamma = 0.5;
  param.cache_size = 16000;

  for(unsigned int r=0; r<norm_data.rows; r++){
    CHECK(problem.push_problem<float>(norm_data.row(r), (double)labels.at<float>(r, 0)));
  }

  bool hard_negative_mining = false;
  if(boost::filesystem::exists(path.c_str())) {
    m->model->read_from_text(path.c_str());
  }else{
    m->model->train(problem, param);
    hard_negative_mining = true;
  }

  int num_iter = 1;
  if(hard_negative_mining){
    param.nr_weight=2;
    param.weight_label = new int[2];
    param.weight = new double[2];
    param.weight_label[0]=-1;
    param.weight_label[1]=+1;
    double Cn = 1;
    double Cp = 10;
    param.weight[0]=Cn;
    param.weight[1]=Cp;

    for(int i=0; i<num_iter; i++){
      int true_pos = 0;
      Mat hard_negs;
      for(unsigned int n=0; n<neg_feat.rows; n++){
        Mat feat = neg_feat.row(n);
        normalize_data(feat, feat, mean, std);
        vector<double> probs;
        float resp = m->model->predict_probability<float>(feat, probs);
        if(resp==-1){
          true_pos++;
        }
        else{
          if(hard_negs.rows == 0){
            hard_negs = feat;
          }else{
            vconcat(hard_negs, feat, hard_negs);
          }

        }
      }
      LOG(INFO)<<path<<"\tIter: "<<i<<"\tAccuracy on all negatvies = "<<true_pos/(float)neg_feat.rows*100;
      LOG(INFO)<<path<<"\tHard negative counts: "<<hard_negs.rows;
      for(unsigned int r=0; r<hard_negs.rows; r++){
        CHECK(problem.push_problem<float>(hard_negs.row(r), -1));
      }
      m->model.reset(new SVM);
      m->model->train(problem, param);
    }
    m->model->write_as_text(path.c_str());
  }

  if(true){
    int true_pos = 0;
    int cor_pos = 0, cor_neg = 0;
    for(unsigned int n=0; n<norm_data.rows; n++){
     vector<double> probs;
     float resp = m->model->predict_probability<float>(norm_data.row(n), probs);
     if(resp==labels.at<float>(n,0)){
      true_pos++;
      if(resp == -1){
        cor_neg++;
      }
      else{
        cor_pos++;
      }
     }
    }
    LOG(INFO)<<path<<"\tAccuracy = "<<true_pos/(float)norm_data.rows*100 << "\tPos:\t" << cor_pos/(float)norm_data.rows*100 << "\tNeg:\t" << cor_neg/(float)norm_data.rows*100;
  }
  neg_feats[pIdx] = cv::Mat();
}


bool learn_param_for_unary_models_mp(vector<vector<Mat> >& appearance_feats,
                                         SPLPI& splp_model,
                                         body_pose::BodyPoseTypes pose_type,
                                         string cache,
                                         int num_threads)
{

  CHECK(appearance_feats.size())<<" Appearance features are not available. ";
  CHECK(appearance_feats[0].size());
  int num_feat = appearance_feats[0][0].row(0).cols - 1; // last value is the check flag

  int total_persons = 0;
  for(unsigned int aIdx=0; aIdx<appearance_feats.size(); ++aIdx){
    total_persons += appearance_feats[aIdx].size();
  }

  vector<Mat> pos_feats((int)pose_type);
  vector<Mat> neg_feats((int)pose_type);

  for(size_t pIdx=0; pIdx<(pose_type); pIdx++){
    pos_feats[pIdx] = Mat::zeros(total_persons, num_feat, CV_32F);
    neg_feats[pIdx] = Mat::zeros(total_persons, num_feat, CV_32F);
  }

  vector<int> p_count((int)pose_type, 0);
  vector<int> n_count((int)pose_type, 0);

  for(size_t aIdx=0; aIdx<appearance_feats.size(); aIdx++){
    for(size_t nIdx=0; nIdx<appearance_feats[aIdx].size(); nIdx++){
      for(size_t pIdx=0; pIdx<(pose_type); pIdx++){

        bool check = static_cast<bool>(appearance_feats[aIdx][nIdx].row(pIdx).at<float>(0,num_feat));

        Mat f =  appearance_feats[aIdx][nIdx].row(pIdx).colRange(0,num_feat);
        if(check){
          f.copyTo(pos_feats[pIdx].row(p_count[pIdx]++));
        }else{
          f.copyTo(neg_feats[pIdx].row(n_count[pIdx]++));
        }
      }
    }
  }

  for(unsigned int pIdx=0; pIdx<(int)pose_type; pIdx++){
    pos_feats[pIdx] = pos_feats[pIdx].rowRange(0, p_count[pIdx]);
    neg_feats[pIdx] = neg_feats[pIdx].rowRange(0, n_count[pIdx]);
  }

  if(num_threads < 1){
    num_threads = ::utils::system::get_available_logical_cpus();
  }

  vector<splpi::BinaryModel> models;
  models.resize((int)pose_type);

  num_threads = 14;
  if(num_threads >= 1){
    boost::thread_pool::executor e(num_threads);
    for(unsigned int pIdx=0; pIdx<(int)pose_type; pIdx++){
      e.submit(boost::bind(&train_unary_svm_mt, pos_feats, neg_feats, &models[pIdx], pIdx, cache));
    }
    e.join_all();
  }
  else{
    for(unsigned int pIdx=0; pIdx<(int)pose_type; pIdx++){
      train_unary_svm_mt(pos_feats, neg_feats, &models[pIdx], pIdx, cache);
    }
  }

  CHECK(splp_model.set_unary_models(models))<<
            "Failed during setting SPLP difference class binary models";
  return true;

}


bool get_appearance_scores(vector<MultiAnnotation> annotations,
                      vector<vector<Mat> >& scores,
                      std::string config_file,
                      std::string cache)
{
  // load if features are precomputed
  string filename = cache+"/features/splp/cnn_features_for_binaries.txt";
  std::ifstream ifs(filename.c_str());

  if(ifs.is_open()){
    LOG(INFO)<<"Already computed features found at "<<filename;
      try{
        boost::archive::text_iarchive ia(ifs);
        ia>>scores;
        LOG(INFO)<<"Features loaded";
        return true;
      }
      catch(boost::archive::archive_exception& ex){
        LOG(INFO)<<"Reload Tree: Archive exception during deserializiation: "
              <<ex.what();
        LOG(INFO)<<"not able to load features from: "<<filename;
      }
  }

  LOG(INFO)<<"Computing features...";
  vector<string> feature_names;
  feature_names += "Mconv5_stage6";
  vision::features::CNNFeatures cnn_feat_extractor(config_file);

  scores.resize(annotations.size());

  boost::progress_display show_progress(annotations.size());
  for(unsigned int aIdx=0; aIdx<annotations.size(); ++aIdx, ++show_progress){
    MultiAnnotation ann = annotations[aIdx];

    Mat orig_image = imread(ann.url);

    scores[aIdx].resize(ann.persons.size());
//    for(unsigned int nIdx=0; nIdx<ann.person_groups.size(); nIdx++){
    for(unsigned int nIdx=0; nIdx<ann.persons.size(); nIdx++){

      int person_id  = nIdx;
//      int person_id  = ann.person_groups[nIdx];

      Annotation s_ann = ann.persons[person_id];

      Mat image;
      float scale = 1.0/s_ann.scale;
      rescale_img(orig_image, image, scale, s_ann);


      Point_<int> center = s_ann.position;

      int offset = 150;
      Rect bbox = Rect(center.x - offset, center.y - offset, 2*offset, 2*offset);
      bbox = vision::geometry_utils::intersect(bbox, Rect(0,0, image.cols, image.rows));

      Mat test_image = image(bbox);

      center = center - Point(bbox.x, bbox.y);

      std::vector<cv::Mat_<float> > cnn_features;
      cnn_feat_extractor.extract_cpm(test_image, feature_names, cnn_features, center, true, false);

      Mat s = Mat::zeros(s_ann.parts.size(), cnn_features.size()+1, CV_32F); // the last value is to specifiy whether the annotation should be used during training or not
      float head_len = get_head_length(s_ann);
      for(unsigned int pIdx=0; pIdx<s_ann.parts.size(); pIdx++){
        Point p = s_ann.parts[pIdx];
        p = p - Point(bbox.x, bbox.y);
        // FIXME: Re-generate annotation files for MPII-Pose dataset to start from index 0
//        p.x--;
//        p.y--;

        if(!is_valid(p) || p.x >= cnn_features[0].cols || p.y >= cnn_features[0].rows){ continue; }
        for(unsigned int cIdx=0; cIdx<cnn_features.size(); cIdx++){
          CHECK_GE(cnn_features[cIdx].at<float>(p), -1.1)<<aIdx<<"\t"<<nIdx<<"\t"<<p.x<<"\t"<<p.y<<"\t"<<cnn_features[0].cols<<"\t"<<cnn_features[0].rows;
          CHECK_LE(cnn_features[cIdx].at<float>(p), 1.1)<<aIdx<<"\t"<<nIdx<<"\t"<<p.x<<"\t"<<p.y<<"\t"<<cnn_features[0].cols<<"\t"<<cnn_features[0].rows;
          s.at<float>(pIdx, cIdx) = cnn_features[cIdx].at<float>(p);
        }

        Point max_loc;
        minMaxLoc(cnn_features[pIdx], 0,0,0,&max_loc);

        float diff = sqrt(pow(max_loc.x - p.x,2) + pow(max_loc.y - p.y, 2));
        s.at<float>(pIdx, cnn_features.size()) = diff <= 0.5*head_len ? 1 : 0; // correct detection or not
      }
      cout<<s<<endl;
      scores[aIdx][nIdx] = s;
    }
  }

  std::ofstream ofs(filename.c_str());
  if(ofs==0){
    LOG(INFO)<<"Error: Cannot open the given path to save detected poses.";

  }
  boost::archive::text_oarchive oa(ofs);
  oa<<scores;
  ofs.flush();
  ofs.close();
  LOG(INFO)<<"Features saved at :"<<filename;
  return true;
}

bool learn_model_parameters_mp(SPLPI& splp_model, string train_file,
                                int patch_size,
                                float scale_factor,
                                body_pose::BodyPoseTypes pose_type,
                                string cache)
{


  vector<MultiAnnotation> annotations;
  //load_multi_annotations(annotations, train_file);

  // multiply the scales ŝof annotations with scale_factor
  /*for(unsigned int aIdx=0; aIdx<annotations.size(); aIdx++){
    MultiAnnotation& ann = annotations[aIdx];
    for(unsigned int nIdx=0; nIdx<ann.persons.size(); nIdx++){
      ann.persons[nIdx].scale *= scale_factor;
    }
  }*/

  string model_dir = cache+"/models";
  if(!boost::filesystem::exists(model_dir)){
    CHECK(boost::filesystem::create_directory(model_dir));
  }

  string splp_dir = model_dir+"/splp";
  if(!boost::filesystem::exists(splp_dir)){
    CHECK(boost::filesystem::create_directory(splp_dir));
  }

  /// load/learn svms for unary normalization
  string unary_model_dir = splp_dir+"/unaries";
  if(!boost::filesystem::exists(unary_model_dir)){
    CHECK(boost::filesystem::create_directory(unary_model_dir));
  }

  string unary_model_path = unary_model_dir+"/models.txt";
  if(boost::filesystem::exists(unary_model_path.c_str())){
    LOG(INFO)<<"Pre-learned unary models found at: "<<unary_model_path;
    splp_model.load_unary_models(unary_model_path);
  }
  else{
    vector<vector<Mat> > scores;
    get_appearance_scores(annotations, scores, "./config_file.txt", cache);
    LOG(INFO)<<"Learning unary models..";
    CHECK(learn_param_for_unary_models_mp(scores, splp_model, pose_type, unary_model_dir));
    splp_model.save_unary_models(unary_model_path);
  }


  /// load/learn same class binary models
  string same_class_model_dir = splp_dir+"/same_class_binaries";
  if(!boost::filesystem::exists(same_class_model_dir)){
    CHECK(boost::filesystem::create_directory(same_class_model_dir));
  }

  string same_class_binary_path = same_class_model_dir+"/models.txt";
  if(boost::filesystem::exists(same_class_binary_path.c_str())){
    LOG(INFO)<<"Pre-learned same class binary models found at: "<<same_class_binary_path;
    splp_model.load_same_class_binary_models(same_class_binary_path);
  }
  else{
    LOG(INFO)<<"Learning same class binary params..";
    CHECK(learn_param_for_same_class_binaries_mp(annotations, splp_model, patch_size, pose_type, same_class_model_dir));
    splp_model.save_same_class_binary_models(same_class_binary_path);
  }

  /// Load/Learn diff class binary models
  string diff_class_model_dir = splp_dir+"/diff_class_binaries";
  if(!boost::filesystem::exists(diff_class_model_dir)){
    CHECK(boost::filesystem::create_directory(diff_class_model_dir));
  }

  string diff_class_binary_path = diff_class_model_dir+"/models.txt";
  if(boost::filesystem::exists(diff_class_binary_path.c_str())){
    LOG(INFO)<<"Pre-learned diff class binary models found at: "<<diff_class_binary_path;
    splp_model.load_diff_class_binary_models(diff_class_binary_path);
  }
  else{
    LOG(INFO)<<"Learning diff class binary params..";
    vector<vector<Mat> > scores;
    get_appearance_scores(annotations, scores, "./config_file.txt", cache);
    CHECK(learn_param_for_diff_class_binaries_mp(annotations, scores, splp_model, pose_type, diff_class_model_dir));
    splp_model.save_diff_class_binary_models(diff_class_binary_path);
  }
  return true;
}

}

}