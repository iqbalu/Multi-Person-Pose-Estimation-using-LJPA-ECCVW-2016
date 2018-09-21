/*
 * LibLinear.hpp
 *
 *  Created on: Oct 25, 2011
 *      Author: lbossard
 */

#ifndef UTILS_LIBSVM_LIBSVM_HPP_
#define UTILS_LIBSVM_LIBSVM_HPP_

#include <vector>

#include <opencv2/core/core.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>

#include <glog/logging.h>

#include "libsvm_common.hpp"
#include "cpp/utils/libsvm/libsvm.hpp"


namespace utils
{
namespace libsvm
{

/*
 * libsvm is a c library (that does not use namespaces). To prevent naming
 * clashes (because of lower case and generic type names) this lib is included
 * into an namespace
 */
namespace wrapped {
#include "cpp/third_party/libsvm/svm.h"
}
typedef wrapped::svm_node FeatureNode;
typedef wrapped::svm_problem Problem;
typedef wrapped::svm_model Model;
typedef wrapped::svm_parameter SolverParameter;

namespace svm_type
{
enum svm_type
{
  C_SVC       = wrapped::C_SVC,
  NU_SVC      = wrapped::NU_SVC,
  ONE_CLASS   = wrapped::ONE_CLASS,
  EPSILON_SVR = wrapped::EPSILON_SVR,
  NU_SVR      = wrapped::NU_SVR
};

typedef svm_type T;
}

namespace kernel_type
{
enum kernel_type
{
  LINEAR       = wrapped::LINEAR,
  POLY         = wrapped::POLY,
  RBF          = wrapped::RBF,
  SIGMOID      = wrapped::SIGMOID,
  PRECOMPUTED  = wrapped::PRECOMPUTED
};
}

class ProblemHolder : boost::noncopyable
{
public:
    typedef std::vector<double> LabelVector;
    typedef std::vector<FeatureNode*> FeatureCollection;

    ProblemHolder();
    virtual ~ProblemHolder();

    /**
     * Pushes a sample to the problem.
     * NOTE: you need to allocate the problem first.
     * @param x
     * @param y
     * @return true on success, false otherwhise. NOTE: if it returns false the problem is in an inconsisstent state
     */
    template <typename T>
    bool push_problem(const cv::Mat_<T>& x, const double y, const cv::Mat_<uchar>& mask= cv::Mat());

    template <typename T, typename F>
    bool push_problem(const cv::Mat_<T>& x, const double y, const F& norm_function, const cv::Mat_<uchar>& mask=cv::Mat());

    template <typename T>
    bool read(const cv::Mat_<T>& x, const std::vector<double>& y);

    template <typename T, typename F>
    bool read(const cv::Mat_<T>& x, const std::vector<double>& y, const F& norm_function);

    bool read(std::istream& stream);

    void allocate(const std::size_t sample_count, const std::size_t non_zero_entries);
    void clear();

    inline const LabelVector& labels() const { return y_;}
    inline const FeatureCollection & features() const { return x_;}
    inline const std::vector<FeatureNode>& feature_nodes() const {return x_features_;};
    inline const std::size_t used_feature_nodes() const {return next_node_idx_;};

    inline const Problem& libsvm_problem() const;

private:
    void init();
    void dynamic_expand_features();
    void dynamic_expand_labels();

    Problem problem_;
    LabelVector y_;
    std::vector<FeatureNode> x_features_;
    std::size_t next_node_idx_;
    FeatureCollection x_; // pointers into x_features

};


class SVMHolder : boost::noncopyable
{
public:

    static const std::vector<double> default_cost;

    SVMHolder();
    virtual ~SVMHolder();

    bool read_from_text(const std::string& file);
    bool write_as_text(const std::string& file) const;


    template <typename T>
    int predict(const cv::Mat_<T>& features, std::vector<double>& values, const cv::Mat_<uchar>& mask=cv::Mat_<uchar>())  const;

    int predict(const cv::Mat_<int>& features, const cv::Mat_<uchar>& mask=cv::Mat_<uchar>())  const;

    int predict(const std::vector<FeatureNode>& features, std::vector<double>& values)  const;

    int predict(const FeatureNode* feature, std::vector<double>& values)  const;

    template <typename T>
    int predict_probability(const cv::Mat_<T>& features, std::vector<double>& values, const cv::Mat_<uchar>& mask=cv::Mat_<uchar>())  const;

    int predict_probability(const cv::Mat_<int>& features, const cv::Mat_<uchar>& mask=cv::Mat_<uchar>())  const;

    int predict_probability(const std::vector<FeatureNode>& features, std::vector<double>& values) const;

    int predict_probability(const FeatureNode* feature, std::vector<double>& values) const;



    void train(
            const ProblemHolder& problem,
            const SolverParameter param_);

    float cross_validation(
                int num_folds,
                const ProblemHolder& problem,
                const SolverParameter param_);

    void train_best_model(
        const ProblemHolder& problem,
        SolverParameter param_,
        const std::vector<double>& costs,
        const int32_t fold_count=5
        );

    void set_defaults(SolverParameter& param);

    inline const Model& get_model() const
    {
        return *model_;
    }

    inline const int class_count() const
    {
        return model_->nr_class;
    }

    int get_num_weights() const;

    inline void get_weights(std::vector<double>& weights ) const {
        weights = weights_;
    }


    ///CATION!
    inline Model& get_model_nonconst() {
        return *model_;
    }
    std::vector<double>& get_weights_nonconst() {
      return weights_;
    }
    std::vector<int>& get_labels_nonconst() {
      return labels_;
    }

private:

    boost::shared_ptr<Model> model_;
    std::vector<double> weights_;
    std::vector<int> labels_;

    void setDefaults(SolverParameter& param);
    void clear();

    void own_model_from_libsvm();

    friend class boost::serialization::access;
    template <class Archive>
    void load(Archive& archive, const unsigned int /*version*/);

    template <class Archive>
    void save(Archive& archive, const unsigned int /*version*/) const;

    BOOST_SERIALIZATION_SPLIT_MEMBER();
};

//------------------------------------------------------------------------------

template <typename T, typename Fun>
void convert_svm_features(
        const cv::Mat_<T>& vec,
        std::vector<FeatureNode>& features,
        const Fun& fun,
        const cv::Mat_<uchar>& mask=cv::Mat())
{
     ::utils::libsvm::convert_features(
                vec,
                features,
                fun,
                mask
                );
}


template <typename T>
void convert_svm_features(
            const cv::Mat_<T>& vec,
            std::vector<FeatureNode>& features,
            const cv::Mat_<uchar>& mask=cv::Mat())
{
    convert_svm_features(
            vec,
            features,
            ::utils::libsvm::IdentityFunction(),
            mask);
}



////////////////////////////////////////////////////////////////////////////////
//
template <typename T>
bool ProblemHolder::read(const cv::Mat_<T>& x, const std::vector<double>& y)
{
    return read(x, ::utils::libsvm::IdentityFunction());
}

template <typename T, typename F>
bool ProblemHolder::read(const cv::Mat_<T>& x, const std::vector<double>& y, const F& norm_function)
{
    if (y.size() != x.rows)
    {
        std::cerr << "Number of features and labels not the same" << std::endl;
        return false;
    }

    const std::size_t vector_count = y.size();

    // determine number of non zero elements
    std::size_t non_zero_elements = cv::countNonZero(x);

    // allocate
    this->allocate(vector_count, non_zero_elements);

    // copy x
    for (int r = 0; r < x.rows; ++r)
    {
        const cv::Mat& row = x.row(r);
        push_problem<T>(row, y[r], norm_function);
    }
    return false;
}

template <typename T>
bool ProblemHolder::push_problem(const cv::Mat_<T>& row,
           const double y, const cv::Mat_<uchar>& mask)
{
    return push_problem(row, y, ::utils::libsvm::IdentityFunction(), mask);
}

template <typename T, typename F>
bool ProblemHolder::push_problem(
      const cv::Mat_<T>& row,
      const double y,
      const F& norm_function,
      const cv::Mat_<uchar>& mask)
{
    // check, if we're full, sample wise...
    if (problem_.l >= y_.size())
    {
        this->dynamic_expand_labels();
    }
    // feature node wise
    if (next_node_idx_ >= x_features_.size())
    {
      this->dynamic_expand_features();
    }

    // save label
    y_[problem_.l] = y;

    // save start away
    x_[problem_.l] = &x_features_[next_node_idx_];

    // copy features
    int max_index = -1;
    for (int c = 0; c < row.cols; ++c)
    {

        if( mask.data && mask(0,c) == 0 )
        {
            continue;
        }

        if (row(c) != static_cast<T>(0) )
        {
            FeatureNode& node = x_features_[next_node_idx_];
            next_node_idx_++;

            node.index = c + 1;
            node.value = norm_function(row(c));

            max_index = std::max(node.index, max_index);

            if (next_node_idx_ >= x_features_.size())
            {
                this->dynamic_expand_features();
            }
        }
    }
    // insert last
    x_features_[next_node_idx_].index = -1;
    next_node_idx_++;

    // increase count
    problem_.l++; // feature count
    //problem_.n = std::max(max_index, problem_.n); // max dimension

    return true;
}
inline const Problem& ProblemHolder::libsvm_problem()const
{
    return problem_;
}
//------------------------------------------------------------------------------
template <class Archive>
void SVMHolder::load(Archive& archive, const unsigned int /*version*/)
{
//    model_.reset(new Model());
//    archive & model_->param;
//    archive & model_->nr_class;
//    archive & model_->nr_feature;
//    archive & model_->bias;
//
//    // we load all the labels
//    archive & labels_;
//    model_->label = const_cast<int*>(labels_.data());
//
//    archive & weights_;
//    model_->w = const_cast<double*>(weights_.data());
}


template <class Archive>
void SVMHolder::save(Archive& archive, const unsigned int /*version*/) const
{
//    archive & model_->param.solver_type;
//    archive & model_->nr_class;
//    archive & model_->nr_feature;
//    archive & model_->bias;
//    archive & labels_;
//    archive & weights_;
}
template <typename T>
int SVMHolder::predict(const cv::Mat_<T>& vec, std::vector<double>& values,  const cv::Mat_<uchar>& mask)  const
{
    std::vector<FeatureNode> features;
    convert_svm_features<T>(vec, features, mask);

    return predict(features, values);
}

template <typename T>
int SVMHolder::predict_probability(const cv::Mat_<T>& vec, std::vector<double>& values,  const cv::Mat_<uchar>& mask)  const
{
    std::vector<FeatureNode> features;
    convert_svm_features<T>(vec, features, mask);

    return predict_probability(features, values);
}

SolverParameter get_default_svm_param();

} /* namespace libsvm */
} /* namespace utils */

#endif /* UTILS_LIBSVM_LIBSVM_HPP_ */
