#include "src/decoder_config.h"

#include "feature_pipeline.h"

using namespace kaldi;

namespace alex_asr {
    FeaturePipeline::FeaturePipeline(DecoderConfig &config) :
        base_feature_(NULL),
        cmvn_(NULL),
        cmvn_state_(NULL),
        splice_(NULL),
        delta_(NULL),
        transform_lda_(NULL),
        ivector_(NULL),
        ivector_append_(NULL),
        pitch_(NULL),
        pitch_feature_(NULL),
        pitch_append_(NULL),
        final_feature_(NULL)

    {
        OnlineFeatureInterface *prev_feature;

        if(config.feature_type == DecoderConfig::MFCC) {
            KALDI_VLOG(3) << "Feature MFCC "
                          << config.mfcc_opts.mel_opts.low_freq
                          << " " << config.mfcc_opts.mel_opts.high_freq;
            prev_feature = base_feature_ = new OnlineMfcc(config.mfcc_opts);
            KALDI_VLOG(3) << "    -> dims: " << base_feature_->Dim();
        } else if(config.feature_type == DecoderConfig::FBANK) {
            KALDI_VLOG(3) << "Feature FBANK "
                          << config.fbank_opts.mel_opts.low_freq
                          << " " << config.fbank_opts.mel_opts.high_freq;
            prev_feature = base_feature_ = new OnlineFbank(config.fbank_opts);
            KALDI_VLOG(3) << "    -> dims: " << base_feature_->Dim();
        } else {
            KALDI_ERR << "You have to specify a valid feature_type.";
        }

        if(config.use_pitch) {
            pitch_ = new OnlinePitchFeature(config.pitch_opts);
            pitch_feature_ = new OnlineProcessPitch(config.pitch_process_opts, pitch_);
            prev_feature = pitch_append_ = new OnlineAppendFeature(prev_feature, pitch_feature_);
        }

        if(config.use_cmvn) {
            KALDI_VLOG(3) << "Feature CMVN";
            cmvn_state_ = new OnlineCmvnState(*config.cmvn_mat);
            prev_feature = cmvn_ = new OnlineCmvn(config.cmvn_opts, *cmvn_state_, prev_feature);
        }

        if(config.model_type != DecoderConfig::NNET3) {
            // TODO
            KALDI_VLOG(3) << "Feature SPLICE " << config.splice_opts.left_context << " " <<
                          config.splice_opts.right_context;
            prev_feature = splice_ = new OnlineSpliceFrames(config.splice_opts, prev_feature);
            KALDI_VLOG(3) << "    -> dims: " << splice_->Dim();
        }

        if(config.cfg_delta != "") {
            KALDI_VLOG(3) << "Feature DELTA";
            prev_feature = delta_ = new OnlineDeltaFeature(config.delta_opts, prev_feature);
            KALDI_VLOG(3) << "    -> dims: " << delta_->Dim();
        }

        if(config.use_lda) {
            KALDI_VLOG(3) << "Feature LDA " << config.lda_mat->NumRows() << " " << config.lda_mat->NumCols();
            prev_feature = transform_lda_ = new OnlineTransform(*config.lda_mat, prev_feature);
            KALDI_VLOG(3) << "    -> dims: " << transform_lda_->Dim();
        }

        if (config.use_ivectors) {
            KALDI_VLOG(3) << "Feature IVectors";
            ivector_ = new OnlineIvectorFeature(*config.ivector_extraction_info, base_feature_);
            prev_feature = ivector_append_ = new OnlineAppendFeature(prev_feature, ivector_);
            KALDI_VLOG(3) << "     -> dims: " << prev_feature->Dim();
        }

        final_feature_ = prev_feature;
    }

    FeaturePipeline::~FeaturePipeline() {
        delete base_feature_;
        delete cmvn_;
        delete cmvn_state_;
        delete splice_;
        delete delta_;
        delete transform_lda_;
        delete ivector_;
        delete ivector_append_;
        delete pitch_;
        delete pitch_feature_;
        delete pitch_append_;
    }

    OnlineFeatureInterface *FeaturePipeline::GetFeature() {
        return final_feature_;
    }

    void FeaturePipeline::AcceptWaveform(BaseFloat sampling_rate,
                                                 const VectorBase<BaseFloat> &waveform) {
        base_feature_->AcceptWaveform(sampling_rate, waveform);
        if(pitch_) {
            pitch_->AcceptWaveform(sampling_rate, waveform);
        }
    }

    void FeaturePipeline::InputFinished() {
        base_feature_->InputFinished();
        if(pitch_) {
            pitch_->InputFinished();
        }
    }

    OnlineIvectorFeature *FeaturePipeline::GetIvectorFeature() {
        return ivector_;
    }
}

