V2X ETSI ASN.1 utility libraries
================================

This repository contains code for transmitting UPER ASN.1 encoded ETSI messages over AMQP connections.

- **v2x_amqp_connector_lib**: Contains code for handling an AMQP connection. Supports message filtering and automatic connection/reconnection management.
- **v2x_etsi_asn1_lib**: Contains ASN.1 message definitions and code to encode/decode,send/receive such messages via AMQP.
- **example**: Contains example code demonstrating how the above libraries can be used.

ASN.1 Definitions
=================

The ASN.1 definitions used in this project are based on the official messages (except for MCM, which is independently developed), but have been modified to fit the research projects at the Institute.

The original definitions can be found here:
 - CDD (https://forge.etsi.org/rep/ITS/asn1/cdd_ts102894_2 branch release2 commit 60799344)
 - CPM release2 (https://forge.etsi.org/rep/ITS/asn1/cpm_ts103324 branch master commit 73043d4f)
 - CAM release2 draft (https://forge.etsi.org/rep/ITS/asn1/cam_ts103900 branch master commit dc61f620)
 - VAM release2 (https://forge.etsi.org/rep/ITS/asn1/vam-ts103300_3 branch master commit c6db4d08)

Summary of changes (diff can be found [here](https://github.com/uulm-mrm/v2x_etsi_asn1/commit/f812f7a86b002a7acebf74427a6a76fb76aaa886)):
 - AngleConfidence, SpeedConfidence: Increased number of bits to express higher variances
 - MatrixIncludedComponents: Allow correlation entries between object dimensions and other state variables
 - PerceivedObject: Added optional associatedStationID field for tracks associated with recognized ITS stations (by matching CAMs to tracks)
 - PerceivedObject: Added container datatype for trajectory predictions (PredictionsContainer)

The MCM definition is based on our paper [An Extended Maneuver Coordination Protocol with Support for Urban Scenarios and Mixed Traffic](https://oparu.uni-ulm.de/xmlui/handle/123456789/42038).

Dependencies
============

 - [CMake](https://cmake.org/)
 - [Apache Proton](https://github.com/apache/qpid-proton)
 - [asn1c](https://github.com/fillabs/asn1c)
 - [date](https://github.com/HowardHinnant/date)
 - [aduulm_cmake_tools](https://github.com/uulm-mrm/aduulm_cmake_tools)
 - [aduulm_logger](https://github.com/uulm-mrm/aduulm_logger)

License
=======

License: Apache 2.0

Author: Jan Strohbeck (UULM-MRM)

Maintainer: Jan Strohbeck (UULM-MRM)

Affiliation: Institute of Measurement, Control and Microtechnology (MRM), Ulm University (UULM)

Acknowledgements
================

The basic versions of the adapted ASN.1 files are based on our joint developments with the project partners within the project LUKAS. This project was financially supported by the Federal Ministry of Economic Affairs and Climate Action of Germany within the program "Highly and Fully Automated Driving in Demanding Driving Situations" (project LUKAS, grant number 19A20004F).

Parts of the further developments have been made as part of the PoDIUM project and the EVENTS project, which both are funded by the European Union under grant agreement No. 101069547 and No. 101069614, respectively. Views and opinions expressed are however those of the authors only and do not necessarily reflect those of the European Union or European Commission. Neither the European Union nor the granting authority can be held responsible for them.

Parts of the further developments have been financially supported by the Federal Ministry of Education and Research (project AUTOtech.*agil*, FKZ 01IS22088W).
