#include "oneflow/core/operator/normalization_op.h"
#include "oneflow/core/kernel/kernel_util.h"

namespace oneflow {

void NormalizationOp::InitFromOpConf() {
  const NormalizationOpConf& conf = op_conf().normalization_conf();
#ifdef WITH_CUDA
  if (DevIsGpuAndEnableCudnn()) { CHECK_GE(conf.epsilon(), CUDNN_BN_MIN_EPSILON); }
#endif
  CHECK_GE(conf.momentum(), 0);
  CHECK_LE(conf.momentum(), 1);
  EnrollInputBn("in");
  EnrollOutputBn("out");
  if (conf.has_moving_mean()) {
    EnrollInputBn("moving_mean")->set_is_mutable(conf.is_training());
  } else {
    EnrollTmpBn("moving_mean");
  }
  if (conf.has_moving_variance()) {
    EnrollInputBn("moving_variance")->set_is_mutable(conf.is_training());
  } else {
    EnrollTmpBn("moving_variance");
  }
  if (conf.has_gamma()) {
    EnrollInputBn("gamma")->set_is_mutable(true);
  } else {
    if (DevIsGpuAndEnableCudnn()) {
      EnrollConstBufBn("gamma");
    } else {
      UNIMPLEMENTED();
    }
  }
  if (conf.has_beta()) {
    EnrollInputBn("beta")->set_is_mutable(true);
  } else {
    if (DevIsGpuAndEnableCudnn()) {
      EnrollConstBufBn("beta");
    } else {
      UNIMPLEMENTED();
    }
  }
  if (conf.is_training()) {
    EnrollOutputBn("mean", false);
    EnrollOutputBn("inv_variance", false);
  }
}

const PbMessage& NormalizationOp::GetCustomizedConf() const {
  return op_conf().normalization_conf();
}

void NormalizationOp::InferBlobDescs(
    std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const ParallelContext* parallel_ctx) const {
  const NormalizationOpConf& conf = op_conf().normalization_conf();
  const BlobDesc* in = GetBlobDesc4BnInOp("in");
  const DataType data_type = in->data_type();
  *GetBlobDesc4BnInOp("out") = *in;
  const Shape param_shape({in->shape().At(conf.axis())});
  const std::function<void(const std::string&)> CheckParamBlobDesc = [&](const std::string& bn) {
    const BlobDesc* blob_desc = GetBlobDesc4BnInOp(bn);
    if (blob_desc != nullptr) {
      CHECK_EQ(blob_desc->data_type(), data_type);
      CHECK_EQ(blob_desc->shape(), param_shape);
    }
  };
  const std::function<void(const std::string&)> SetParamBlobDesc = [&](const std::string& bn) {
    BlobDesc* blob_desc = GetBlobDesc4BnInOp(bn);
    if (blob_desc != nullptr) {
      blob_desc->set_data_type(data_type);
      blob_desc->mut_shape() = param_shape;
    }
  };
  (conf.has_moving_mean() ? CheckParamBlobDesc : SetParamBlobDesc)("moving_mean");
  (conf.has_moving_variance() ? CheckParamBlobDesc : SetParamBlobDesc)("moving_variance");
  (conf.has_beta() ? CheckParamBlobDesc : SetParamBlobDesc)("beta");
  (conf.has_gamma() ? CheckParamBlobDesc : SetParamBlobDesc)("gamma");
  if (conf.is_training()) {
    SetParamBlobDesc("mean");
    SetParamBlobDesc("inv_variance");
  }
}

void NormalizationOp::InferHasBatchDim(
    std::function<bool*(const std::string&)> HasBatchDim4BnInOp) const {
  *HasBatchDim4BnInOp("out") = *HasBatchDim4BnInOp("in");
  *HasBatchDim4BnInOp("mean") = false;
  *HasBatchDim4BnInOp("inv_variance") = false;
}

void NormalizationOp::GetSbpSignatures(
    const std::function<const BlobDesc&(const std::string&)>& LogicalBlobDesc4Ibn,
    SbpSignatureList* sbp_sig_list) const {
  SbpSignatureBuilder()
      .Broadcast(input_bns())
      .Broadcast(output_bns())
      .Split("in", 0)
      .Split("out", 0)
      .Build(sbp_sig_list->mutable_sbp_signature()->Add());
}

REGISTER_OP(OperatorConf::kNormalizationConf, NormalizationOp);

}  // namespace oneflow
