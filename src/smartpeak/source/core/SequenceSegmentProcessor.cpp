// TODO: Add copyright

#include <SmartPeak/core/SequenceSegmentProcessor.h>
#include <SmartPeak/core/Filenames.h>
#include <SmartPeak/core/MetaDataHandler.h>
#include <SmartPeak/core/SequenceHandler.h>
#include <SmartPeak/io/OpenMSFile.h>
#include <OpenMS/ANALYSIS/QUANTITATION/AbsoluteQuantitation.h>
#include <OpenMS/METADATA/AbsoluteQuantitationStandards.h>

namespace SmartPeak
{
  void SequenceSegmentProcessor::getSampleIndicesBySampleType(
    const SequenceSegmentHandler& sequenceSegmentHandler,
    const SequenceHandler& sequenceHandler,
    const MetaDataHandler::SampleType sampleType,
    std::vector<size_t>& sampleIndices
  )
  {
    sampleIndices.clear();
    for (const size_t index : sequenceSegmentHandler.getSampleIndices()) {
      if (sequenceHandler.getSequence().at(index).getMetaData().getSampleType() == sampleType) {
        sampleIndices.push_back(index);
      }
    }
  }

  void SequenceSegmentProcessor::optimizeCalibrationCurves(
    SequenceSegmentHandler& sequenceSegmentHandler_IO,
    const SequenceHandler& sequenceHandler_I,
    const std::vector<std::map<std::string, std::string>>& AbsoluteQuantitation_params_I,
    const bool verbose_I
  )
  {
    if (verbose_I) {
      std::cout << "==== START optimizeCalibrationCurves" << std::endl;
    }

    std::vector<size_t> standards_indices;

    // get all standards
    getSampleIndicesBySampleType(
      sequenceSegmentHandler_IO,
      sequenceHandler_I,
      MetaDataHandler::SampleType::Standard,
      standards_indices
    );

    // check if there are any standards to calculate the calibrators from
    if (standards_indices.empty()) {
      std::cout << "standards_indices argument is empty. Returning." << std::endl;
      return;
    }

    std::vector<OpenMS::FeatureMap> standards_featureMaps;
    for (const size_t index : standards_indices) {
      standards_featureMaps.push_back(sequenceHandler_I.getSequence().at(index).getRawData().getFeatureMap());
    }

    if (AbsoluteQuantitation_params_I.empty()) {
      std::cout << "AbsoluteQuantitation_params_I argument is empty. Returning." << std::endl;
      return;
    }

    // add in the method parameters
    OpenMS::AbsoluteQuantitation absoluteQuantitation;
    OpenMS::Param parameters = absoluteQuantitation.getParameters();
    Utilities::updateParameters(parameters, AbsoluteQuantitation_params_I);
    absoluteQuantitation.setParameters(parameters);

    absoluteQuantitation.setQuantMethods(sequenceSegmentHandler_IO.getQuantitationMethods());

    std::map<std::string, std::vector<OpenMS::AbsoluteQuantitationStandards::featureConcentration>> components_to_concentrations;

    for (const OpenMS::AbsoluteQuantitationMethod& row : sequenceSegmentHandler_IO.getQuantitationMethods()) {
      // map standards to features
      OpenMS::AbsoluteQuantitationStandards absoluteQuantitationStandards;
      std::vector<OpenMS::AbsoluteQuantitationStandards::featureConcentration> feature_concentrations;

      absoluteQuantitationStandards.getComponentFeatureConcentrations(
        sequenceSegmentHandler_IO.getStandardsConcentrations(),
        standards_featureMaps,
        row.getComponentName(),
        feature_concentrations
      );

      // remove features with an actual concentration of 0.0 or less
      std::vector<OpenMS::AbsoluteQuantitationStandards::featureConcentration> feature_concentrations_pruned;
      for (const OpenMS::AbsoluteQuantitationStandards::featureConcentration& feature : feature_concentrations) {
        if (feature.actual_concentration > 0.0) {
          feature_concentrations_pruned.push_back(feature);
        }
      }

      // remove components without any points
      if (feature_concentrations_pruned.empty())
        continue;

      // find the optimal calibration curve for each component
      absoluteQuantitation.optimizeSingleCalibrationCurve(
        row.getComponentName(),
        feature_concentrations_pruned
      );

      components_to_concentrations.erase(row.getComponentName());
      components_to_concentrations.insert({row.getComponentName(), feature_concentrations_pruned});
    }

    // store results
    sequenceSegmentHandler_IO.setComponentsToConcentrations(components_to_concentrations);
    sequenceSegmentHandler_IO.setQuantitationMethods(absoluteQuantitation.getQuantMethods());

    if (verbose_I) {
      std::cout << "==== END   optimizeCalibrationCurves" << std::endl;
    }
  }

  // void SequenceSegmentProcessor::plotCalibrators(
  //   const SequenceSegmentHandler& sequenceSegmentHandler_I,
  //   const std::string& calibrators_pdf_o,
  //   const std::vector<std::map<std::string, std::string>>& SequenceSegmentPlotter_params_I,
  //   const bool verbose_I
  // )
  // {
  //   if (verbose_I)
  //     std::cout << "Plotting calibrators." << std::endl;

  //   if (SequenceSegmentPlotter_params_I.empty() || calibrators_pdf_o.empty())
  //     throw std::invalid_argument("Parameters or filename are empty.");

  //   // TODO: Uncomment when SequenceSegmentPlotter is implemented
  //   SequenceSegmentPlotter sequenceSegmentPlotter;
  //   sequenceSegmentPlotter.setParameters(SequenceSegmentPlotter_params_I);
  //   sequenceSegmentPlotter.plotCalibrationPoints(calibrators_pdf_o, sequenceSegmentHandler_I);
  // }

  void SequenceSegmentProcessor::processSequenceSegment(
    SequenceSegmentHandler& sequenceSegmentHandler_IO,
    SequenceHandler& sequenceHandler_IO,
    const SeqSegProcMethod sequence_segment_processing_event,
    const std::map<std::string, std::vector<std::map<std::string, std::string>>>& parameters,
    const Filenames& filenames,
    const bool verbose_I
  )
  {
    if (sequence_segment_processing_event == CALCULATE_CALIBRATION) {
      // optimize the calibrators
      optimizeCalibrationCurves(
        sequenceSegmentHandler_IO,
        sequenceHandler_IO,
        parameters.at("AbsoluteQuantitation"),
        verbose_I
      );
      // update each sample in the sequence segment with the updated quantitationMethods
      for (const size_t index : sequenceSegmentHandler_IO.getSampleIndices()) {
        sequenceHandler_IO
          .getSequence()
          .at(index)
          .getRawData()
          .setQuantitationMethods(sequenceSegmentHandler_IO.getQuantitationMethods());
      }
    } else if (sequence_segment_processing_event == STORE_QUANTITATION_METHODS) {
      OpenMSFile::storeQuantitationMethods(sequenceSegmentHandler_IO, filenames.quantitationMethods_csv_o, verbose_I);
    } else if (sequence_segment_processing_event == LOAD_QUANTITATION_METHODS) {
      OpenMSFile::loadQuantitationMethods(sequenceSegmentHandler_IO, filenames.quantitationMethods_csv_i, verbose_I);
    } else {
      throw std::invalid_argument("Sequence segment processing event was not recognized. Value: " +
        std::to_string(sequence_segment_processing_event) + "\n");
    }
  }

  std::vector<SequenceSegmentProcessor::SeqSegProcMethod>
  SequenceSegmentProcessor::getDefaultSequenceSegmentProcessingWorkflow(
    const MetaDataHandler::SampleType sample_type
  )
  {
    switch (sample_type) {
      case MetaDataHandler::SampleType::Unknown:
      case MetaDataHandler::SampleType::Blank:
      case MetaDataHandler::SampleType::DoubleBlank:
        return {};
      case MetaDataHandler::SampleType::Standard:
        return { CALCULATE_CALIBRATION };
      case MetaDataHandler::SampleType::QC:
        return {};
        // return { CALCULATE_VARIABILITY };
      case MetaDataHandler::SampleType::Solvent:
        return {};
        // return { CALCULATE_CARRYOVER };
      default:
        throw "Sample Type case not handled.\n";
    }
  }

  bool SequenceSegmentProcessor::checkSequenceSegmentProcessing(
    const std::vector<std::string>& sequence_segment_processing
  )
  {
    const std::set<std::string> valid_events = {
      "calculate_calibration",
      "calculate_carryover",
      "calculate_variability",
      "store_quantitation_methods",
      "load_quantitation_methods",
      "store_components_to_concentrations",
      "plot_calibrators"
    };
    bool is_valid = true;
    for (const std::string& event : sequence_segment_processing) {
      if (0 == valid_events.count(event)) {
        std::cout << "Sequence group processing event '" << event << "' is not valid." << std::endl;
        is_valid = false;
      }
    }
    return is_valid;
  }
}