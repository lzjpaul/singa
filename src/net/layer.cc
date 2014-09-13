// Copyright © 2014 Wei Wang. All Rights Reserved.
// 2014-07-06 15:19

#include <glog/logging.h>
#include <memory>
#include <random>
#include "mshadow/tensor.h"


#include "net/layer.h"
#include "net/data_layer.h"
#include "net/linear_layer.h"
#include "net/relu_layer.h"
#include "net/prediction_layer.h"

namespace lapis {
/*****************************************************************************
 * Implementation for Layer
 *****************************************************************************/
void Layer::Init(const LayerProto &proto) {
  name_ = proto.name();
  drop_prob_=proto.drop_prob();
  type_=proto.type();
}

void Layer::ToProto(LayerProto *proto) {
  proto->set_name(name_);
}
void Layer::Setup(const char flag){
  if(drop_prob_>0) {
    in_edges_[0]->SetupTopBlob(AllocData(flag),&drop_fea_);
    in_edges_[0]->SetupTopBlob(AllocData(flag),&drop_grad_);
    in_edges_[0]->SetupTopBlob(AllocData(flag),&mask_);
    VLOG(2)<<name_<<" drop prob is "<<drop_prob_<<drop_fea_.tostring();
  }
}
void Layer::Forward() {
  VLOG(1)<<"forward layer "<<name_;
}

void Layer::Backward() {
  VLOG(1)<<"backward layer "<<name_;
}

void Layer::Dropout(float drop_prob, const Blob &src, Blob *dest, Blob *mask) {
  Random &rnd = Lapis::Instance()->rnd();
  // with 1-drop_prob to keep one neuron, i.e., mask=1
  float keep_prob = 1.0 - drop_prob;
  int len = dest->length();
  Tensor1 dest_t(dest->dptr, Shape1(len));
  Tensor1 mask_t(mask->dptr, Shape1(len));
  Tensor1 src_t(src.dptr, Shape1(len));
//  mask_t = mshadow::expr::F<mshadow::op::threshold>(rnd.uniform(mask_t.shape), keep_prob);
  rnd.SampleUniform(mask_t);
  float* maskdptr=mask->dptr;
  for(int i=0;i<len;i++)
    maskdptr[i]=maskdptr[i]<=keep_prob?1.0f:0.0f;
  dest_t = src_t * mask_t *(1.0 / keep_prob);
}

void Layer::ComputeDropoutGradient(const Blob &src ,
                                   const Blob &mask, Blob *dest) {
  int len = dest->length();
  Tensor1 dest_t(dest->dptr, Shape1(len));
  Tensor1 mask_t(mask.dptr, Shape1(len));
  Tensor1 src_t(src.dptr, Shape1(len));
  dest_t = src_t * mask_t;
}
// Currently layers do not have parameters
void Layer::ComputeParamUpdates(const Trainer *trainer) {
  VLOG(1) << "Layer " << name_ << " has no parameters to update\n";
}

/*****************************************************************************
 * Implementation for LayerFactory
 ****************************************************************************/
#define CreateLayer(LayerClass) [](void)->Layer* {return new LayerClass();}
std::shared_ptr<LayerFactory> LayerFactory::instance_;
std::shared_ptr<LayerFactory> LayerFactory::Instance() {
  if (!instance_.get()) {
    instance_.reset(new  LayerFactory());
  }
  return instance_;
}

LayerFactory::LayerFactory() {
  RegisterCreateFunction("DataLayer", CreateLayer(DataLayer));
  RegisterCreateFunction("LinearLayer", CreateLayer(LinearLayer));
  RegisterCreateFunction("ReLULayer", CreateLayer(ReLULayer));
  RegisterCreateFunction("SoftmaxPredictionLayer", CreateLayer(SoftmaxPredictionLayer));
}

void LayerFactory::RegisterCreateFunction(
  const std::string id,
  std::function<Layer*(void)> create_function) {
  layer_map_[id] = create_function;
}

Layer *LayerFactory::Create(const std::string id) {
  CHECK(layer_map_.find(id) != layer_map_.end())
      << "The initialization function " << id << " has not been registered";
  return layer_map_[id]();
}

}  // namespace lapis
