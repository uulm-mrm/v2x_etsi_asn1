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

Summary of changes:
 - CDD: Increased range of variances
 - CPM: Added container datatype for trajectory predictions

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

Author: Jan Strohbeck (MRM)

Maintainer: Jan Strohbeck (MRM)

Affiliation: Institute of Measurement, Control and Microtechnology (MRM), Ulm University
