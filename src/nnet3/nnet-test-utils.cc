// nnet3/nnet-test-utils.cc

// Copyright      2015  Johns Hopkins University (author: Daniel Povey)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include <iterator>
#include <sstream>
#include "nnet3/nnet-test-utils.h"
#include "nnet3/nnet-utils.h"

namespace kaldi {
namespace nnet3 {


// A super-simple case that is just a single affine component, no nonlinearity,
// no splicing.
void GenerateConfigSequenceSimplest(
    const NnetGenerationOptions &opts,
    std::vector<std::string> *configs) {
  std::ostringstream os;

  int32 input_dim = 10 + Rand() % 20,
       output_dim = 100 + Rand() % 200;

  
  os << "component name=affine1 type=AffineComponent input-dim="
     << input_dim << " output-dim=" << output_dim << std::endl;

  os << "input-node name=input dim=" << input_dim << std::endl;  
  os << "component-node name=affine1_node component=affine1 input=input\n";
  os << "output-node name=output input=affine1_node\n";
  configs->push_back(os.str());
}

// A setup with context and an affine component, but no nonlinearity.
void GenerateConfigSequenceSimpleContext(
    const NnetGenerationOptions &opts,
    std::vector<std::string> *configs) {
  std::ostringstream os;

  std::vector<int32> splice_context;
  for (int32 i = -5; i < 4; i++)
    if (Rand() % 3 == 0)
      splice_context.push_back(i);
  if (splice_context.empty())
    splice_context.push_back(0);
  
  int32 input_dim = 10 + Rand() % 20,
      spliced_dim = input_dim * splice_context.size(),
       output_dim = 100 + Rand() % 200;

  os << "component name=affine1 type=AffineComponent input-dim="
     << spliced_dim << " output-dim=" << output_dim << std::endl;

  os << "input-node name=input dim=" << input_dim << std::endl;

  os << "component-node name=affine1_node component=affine1 input=Append(";
  for (size_t i = 0; i < splice_context.size(); i++) {
    int32 offset = splice_context[i];
    os << "Offset(input, " << offset << ")";
    if (i + 1 < splice_context.size())
      os << ", ";
  }
  os << ")\n";
  os << "output-node name=output input=affine1_node\n";
  configs->push_back(os.str());
}



// A simple case, just to get started.
// Generate a single config with one input, splicing, and one hidden layer.
// Also sometimes generate a part of the config that adds a new hidden layer.
void GenerateConfigSequenceSimple(
    const NnetGenerationOptions &opts,
    std::vector<std::string> *configs) {
  std::ostringstream os;

  std::vector<int32> splice_context;
  for (int32 i = -5; i < 4; i++)
    if (Rand() % 3 == 0)
      splice_context.push_back(i);
  if (splice_context.empty())
    splice_context.push_back(0);
  
  int32 input_dim = 10 + Rand() % 20,
      spliced_dim = input_dim * splice_context.size(),
      output_dim = 100 + Rand() % 200,
      hidden_dim = 40 + Rand() % 50;
  bool use_final_nonlinearity = (opts.allow_final_nonlinearity &&
                                 RandInt(0, 1) == 0);
  os << "component name=affine1 type=NaturalGradientAffineComponent input-dim="
     << spliced_dim << " output-dim=" << hidden_dim << std::endl;
  os << "component name=relu1 type=RectifiedLinearComponent dim="
     << hidden_dim << std::endl;
  os << "component name=final_affine type=NaturalGradientAffineComponent input-dim="
     << hidden_dim << " output-dim=" << output_dim << std::endl;
  if (use_final_nonlinearity) {
    if (Rand() % 2 == 0) {
      os << "component name=logsoftmax type=SoftmaxComponent dim="
         << output_dim << std::endl;
    } else {
      os << "component name=logsoftmax type=LogSoftmaxComponent dim="
         << output_dim << std::endl;
    }
  }      
  os << "input-node name=input dim=" << input_dim << std::endl;
  
  os << "component-node name=affine1_node component=affine1 input=Append(";
  for (size_t i = 0; i < splice_context.size(); i++) {
    int32 offset = splice_context[i];
    os << "Offset(input, " << offset << ")";
    if (i + 1 < splice_context.size())
      os << ", ";
  }
  os << ")\n";
  os << "component-node name=nonlin1 component=relu1 input=affine1_node\n";
  os << "component-node name=final_affine component=final_affine input=nonlin1\n";
  if (use_final_nonlinearity) {
    os << "component-node name=output_nonlin component=logsoftmax input=final_affine\n";
    os << "output-node name=output input=output_nonlin\n";
  } else {
    os << "output-node name=output input=final_affine\n";
  }
  configs->push_back(os.str());

  if ((Rand() % 2) == 0) {
    std::ostringstream os2;
    os2 << "component name=affine2 type=NaturalGradientAffineComponent input-dim="
        << hidden_dim << " output-dim=" << hidden_dim << std::endl;
    os2 << "component name=relu2 type=RectifiedLinearComponent dim="
        << hidden_dim << std::endl;
    // regenerate the final_affine component when we add the new config.
    os2 << "component name=final_affine type=NaturalGradientAffineComponent input-dim="
        << hidden_dim << " output-dim=" << output_dim << std::endl;
    os2 << "component-node name=affine2 component=affine2 input=nonlin1\n";
    os2 << "component-node name=relu2 component=relu2 input=affine2\n";
    os2 << "component-node name=final_affine component=final_affine input=relu2\n";
    configs->push_back(os2.str());
  }
}


// This generates a single config corresponding to an RNN.
void GenerateConfigSequenceRnn(
    const NnetGenerationOptions &opts,
    std::vector<std::string> *configs) {
  std::ostringstream os;

  std::vector<int32> splice_context;
  for (int32 i = -5; i < 4; i++)
    if (Rand() % 3 == 0)
      splice_context.push_back(i);
  if (splice_context.empty())
    splice_context.push_back(0);
  
  int32 input_dim = 10 + Rand() % 20,
      spliced_dim = input_dim * splice_context.size(),
      output_dim = 100 + Rand() % 200,
      hidden_dim = 40 + Rand() % 50;
  os << "component name=affine1 type=NaturalGradientAffineComponent input-dim="
     << spliced_dim << " output-dim=" << hidden_dim << std::endl;
  os << "component name=nonlin1 type=RectifiedLinearComponent dim="
     << hidden_dim << std::endl;
  os << "component name=recurrent_affine1 type=NaturalGradientAffineComponent input-dim="
     << hidden_dim << " output-dim=" << hidden_dim << std::endl;
  os << "component name=affine2 type=NaturalGradientAffineComponent input-dim="
     << hidden_dim << " output-dim=" << output_dim << std::endl;
  os << "component name=logsoftmax type=LogSoftmaxComponent dim="
     << output_dim << std::endl;
  os << "input-node name=input dim=" << input_dim << std::endl;
  
  os << "component-node name=affine1_node component=affine1 input=Append(";
  for (size_t i = 0; i < splice_context.size(); i++) {
    int32 offset = splice_context[i];
    os << "Offset(input, " << offset << ")";
    if (i + 1 < splice_context.size())
      os << ", ";
  }
  os << ")\n";
  os << "component-node name=recurrent_affine1 component=recurrent_affine1 "
        "input=Offset(nonlin1, -1)\n";
  os << "component-node name=nonlin1 component=nonlin1 "
        "input=Sum(affine1_node, IfDefined(recurrent_affine1))\n";
  os << "component-node name=affine2 component=affine2 input=nonlin1\n";
  os << "component-node name=output_nonlin component=logsoftmax input=affine2\n";
  os << "output-node name=output input=output_nonlin\n";
  configs->push_back(os.str());
}


void GenerateConfigSequence(
    const NnetGenerationOptions &opts,
    std::vector<std::string> *configs) {
start:
  int32 network_type = RandInt(0, 3);
  switch(network_type) {
    case 0:
      GenerateConfigSequenceSimplest(opts, configs);
      break;
    case 1:
      if (!opts.allow_context)
        goto start;
      GenerateConfigSequenceSimpleContext(opts, configs);
      break;
    case 2:
      if (!opts.allow_context || !opts.allow_nonlinearity)
        goto start;        
      GenerateConfigSequenceSimple(opts, configs);
      break;
    case 3:
      if (!opts.allow_recursion || !opts.allow_context ||
          !opts.allow_nonlinearity)
        goto start;
      GenerateConfigSequenceRnn(opts, configs);
      break;
    default:
      KALDI_ERR << "Error generating config sequence.";
  }
}

void ComputeExampleComputationRequestSimple(
    const Nnet &nnet,
    ComputationRequest *request,
    std::vector<Matrix<BaseFloat> > *inputs) {
  KALDI_ASSERT(IsSimpleNnet(nnet));

  int32 left_context, right_context;
  ComputeSimpleNnetContext(nnet, &left_context, &right_context);

  int32 num_output_frames = 1 + Rand() % 10,
      output_start_frame = Rand() % 10,
      num_examples = 1 + Rand() % 10,
      output_end_frame = output_start_frame + num_output_frames,
      input_start_frame = output_start_frame - left_context - (Rand() % 3),
      input_end_frame = output_end_frame + right_context + (Rand() % 3),
      n_offset = Rand() % 2;
  bool need_deriv = (Rand() % 2 == 0);
  
  request->inputs.clear();
  request->outputs.clear();
  inputs->clear();
  
  std::vector<Index> input_indexes, ivector_indexes, output_indexes;
  for (int32 n = n_offset; n < n_offset + num_examples; n++) {
    for (int32 t = input_start_frame; t < input_end_frame; t++)
      input_indexes.push_back(Index(n, t, 0));
    for (int32 t = output_start_frame; t < output_end_frame; t++)
      output_indexes.push_back(Index(n, t, 0));
    ivector_indexes.push_back(Index(n, 0, 0));
  }
  request->outputs.push_back(IoSpecification("output", output_indexes));
  if (need_deriv || (Rand() % 3 == 0))
    request->outputs.back().has_deriv = true;
  request->inputs.push_back(IoSpecification("input", input_indexes));
  if (need_deriv && (Rand() % 2 == 0))
    request->inputs.back().has_deriv = true;
  int32 input_dim = nnet.InputDim("input");
  KALDI_ASSERT(input_dim > 0);
  inputs->push_back(
      Matrix<BaseFloat>((input_end_frame - input_start_frame) * num_examples,
                        input_dim));
  inputs->back().SetRandn();
  int32 ivector_dim = nnet.InputDim("ivector");  // may not exist.
  if (ivector_dim != -1) {
    request->inputs.push_back(IoSpecification("ivector", ivector_indexes));
    inputs->push_back(Matrix<BaseFloat>(num_examples, ivector_dim));
    inputs->back().SetRandn();
    if (need_deriv && (Rand() % 2 == 0))
      request->inputs.back().has_deriv = true;
  }
  if (Rand() % 2 == 0)
    request->need_model_derivative = need_deriv;
  if (Rand() % 2 == 0)
    request->store_component_stats = true;
}


static void GenerateRandomComponentConfig(std::string *component_type,
                                          std::string *config) {
  int32 n = RandInt(0, 13);
  std::ostringstream os;
  switch(n) {
    case 0: {
      *component_type = "PnormComponent";
      int32 output_dim = RandInt(1, 50), group_size = RandInt(1, 15),
             input_dim = output_dim * group_size;
      os << "input-dim=" << input_dim << " output-dim=" << output_dim;
      break;
    }
    case 1: {
      *component_type = "NormalizeComponent";
      os << "dim=" << RandInt(1, 50);
      break;
    }
    case 2: {
      *component_type = "SigmoidComponent";
      os << "dim=" << RandInt(1, 50);
      break;
    }
    case 3: {
      *component_type = "TanhComponent";
      os << "dim=" << RandInt(1, 50);
      break;
    }
    case 4: {
      *component_type = "RectifiedLinearComponent";
      os << "dim=" << RandInt(1, 50);
      break;
    }
    case 5: {
      *component_type = "SoftmaxComponent";
      os << "dim=" << RandInt(1, 50);
      break;
    }
    case 6: {
      *component_type = "LogSoftmaxComponent";
      os << "dim=" << RandInt(1, 50);
      break;
    }
    case 7: {
      *component_type = "NoOpComponent";
      os << "dim=" << RandInt(1, 50);
      break;
    }
    case 8: {
      *component_type = "FixedAffineComponent";
      int32 input_dim = RandInt(1, 50), output_dim = RandInt(1, 50);
      os << "input-dim=" << input_dim << " output-dim=" << output_dim;
      break;
    }
    case 9: {
      *component_type = "AffineComponent";
      int32 input_dim = RandInt(1, 50), output_dim = RandInt(1, 50);
      os << "input-dim=" << input_dim << " output-dim=" << output_dim;
      break;
    }
    case 10: {
      *component_type = "NaturalGradientAffineComponent";
      int32 input_dim = RandInt(1, 50), output_dim = RandInt(1, 50);
      os << "input-dim=" << input_dim << " output-dim=" << output_dim;
      break;
    }
    case 11: {
      *component_type = "SumGroupComponent";
      std::vector<int32> sizes;
      int32 num_groups = RandInt(1, 50);
      os << "sizes=";
      for (int32 i = 0; i < num_groups; i++) {
        os << RandInt(1, 5);
        if (i + 1 < num_groups)
          os << ',';
      }
      break;
    }
    case 12: {
      *component_type = "FixedScaleComponent";
      os << "dim=" << RandInt(1, 100);
      break;
    }
    case 13: {
      *component_type = "FixedBiasComponent";
      os << "dim=" << RandInt(1, 100);
      break;
    }
    default:
      KALDI_ERR << "Error generating random component";
  }
  *config = os.str();
}


/// Generates random simple component for testing.
Component *GenerateRandomSimpleComponent() {
  std::string component_type, config;
  GenerateRandomComponentConfig(&component_type, &config);
  ConfigLine config_line;
  if (!config_line.ParseLine(config))
    KALDI_ERR << "Bad config line " << config;
  
  Component *c = Component::NewComponentOfType(component_type);
  if (c == NULL)
    KALDI_ERR << "Invalid component type " << component_type;
  c->InitFromConfig(&config_line);
  return c;
}

bool NnetParametersAreIdentical(const Nnet &nnet1,
                                const Nnet &nnet2,
                                BaseFloat threshold = 1.0e-05) {
  KALDI_ASSERT(nnet1.NumComponents() == nnet2.NumComponents());
  int32 num_components = nnet1.NumComponents();
  for (int32 c = 0; c < num_components; c++) {
    const Component *c1 = nnet1.GetComponent(c),
                    *c2 = nnet2.GetComponent(c);
    KALDI_ASSERT(c1->Type() == c2->Type());
    if (c1->Properties() & kUpdatableComponent) {
      const UpdatableComponent *u1 = dynamic_cast<const UpdatableComponent*>(c1),
                               *u2 = dynamic_cast<const UpdatableComponent*>(c2);
      KALDI_ASSERT(u1 != NULL && u2 != NULL);
      BaseFloat prod11 = u1->DotProduct(*u1), prod12 = u1->DotProduct(*u2),
                prod21 = u2->DotProduct(*u1), prod22 = u2->DotProduct(*u2);
      BaseFloat max_prod = std::max(std::max(prod11, prod12),
                                    std::max(prod21, prod22)),
                min_prod = std::min(std::min(prod11, prod12),
                                    std::min(prod21, prod22));
      if (max_prod - min_prod > threshold * max_prod) {
        KALDI_WARN << "Component '" << nnet1.GetComponentName(c)
                   << "' differs in nnet1 versus nnet2: prod(11,12,21,22) = "
                   << prod11 << ',' << prod12 << ',' << prod21 << ',' << prod22;
        return false;
      }
    }
  }
  return true;
}

void GenerateSimpleNnetTrainingExample(
    int32 num_supervised_frames,
    int32 left_context,
    int32 right_context,
    int32 output_dim,
    int32 input_dim,
    int32 ivector_dim,
    NnetExample *example) {
  KALDI_ASSERT(num_supervised_frames > 0 && left_context >= 0 &&
               right_context >= 0 && output_dim > 0 && input_dim > 0
               && example != NULL);
  example->io.clear();
  
  int32 feature_t_begin = RandInt(0, 2);
  int32 num_feat_frames = left_context + right_context + num_supervised_frames;
  Matrix<BaseFloat> input_mat(num_feat_frames, input_dim);
  input_mat.SetRandn();
  NnetIo input_feat("input", feature_t_begin, input_mat);
  if (RandInt(0, 1) == 0)
    input_feat.features.Compress();
  example->io.push_back(input_feat);

  if (ivector_dim > 0) {
    // Create a feature for the iVectors.  iVectors always have t=0 in the
    // current framework.
    Matrix<BaseFloat> ivector_mat(1, ivector_dim);
    ivector_mat.SetRandn();
    NnetIo ivector_feat("ivector", 0, ivector_mat);
    if (RandInt(0, 1) == 0)
      ivector_feat.features.Compress();
    example->io.push_back(ivector_feat);
  }

  {  // set up the output supervision.
    Posterior labels(num_supervised_frames);
    for (int32 t = 0; t < num_supervised_frames; t++) {
      int32 num_labels = RandInt(1, 3);
      BaseFloat remaining_prob_mass = 1.0;
      for (int32 i = 0; i < num_labels; i++) {
        BaseFloat this_prob = (i+1 == num_labels ? 1.0 : RandUniform()) *
            remaining_prob_mass;
        remaining_prob_mass -= this_prob;
        labels[t].push_back(std::pair<int32, BaseFloat>(RandInt(0, output_dim-1),
                                                        this_prob));
      }
    }
    int32 supervision_t_begin = feature_t_begin + left_context;
    NnetIo output_sup("output", output_dim, supervision_t_begin,
                      labels);
    example->io.push_back(output_sup);
  }
}

bool ExampleApproxEqual(const NnetExample &eg1,
                        const NnetExample &eg2,
                        BaseFloat delta) {
  if (eg1.io.size() != eg2.io.size())
    return false;
  for (size_t i = 0; i < eg1.io.size(); i++) {
    NnetIo io1 = eg1.io[i], io2 = eg2.io[i];
    if (io1.name != io2.name || io1.indexes != io2.indexes)
      return false;
    Matrix<BaseFloat> feat1, feat2;
    io1.features.GetMatrix(&feat1);
    io2.features.GetMatrix(&feat2);
    if (!ApproxEqual(feat1, feat2, delta))
      return false;
  }
  return true;
}


} // namespace nnet3
} // namespace kaldi