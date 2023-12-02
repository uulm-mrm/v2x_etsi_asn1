#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
# Start mrm-amqp-broker if not already running
if [ "$( docker container inspect -f '{{.State.Running}}' "mrm-amqp-broker" 2>/dev/null )" != "true" ]; then
  echo "Building AMQP 1.0 broker"
  docker build $SCRIPT_DIR -t mecs3:5000/mrm_broker:latest -f $SCRIPT_DIR/Dockerfile || exit 1
  echo "Starting AMQP 1.0 broker"
  docker run \
    -d \
    --rm \
    --name mrm-amqp-broker \
    -p 127.0.0.1:5672:5672 \
    mecs3:5000/mrm_broker:latest || exit 1
  echo "Waiting until AMQP 1.0 broker is up..."
  sleep 5
  echo "Creating topic..."
  docker exec -it mrm-amqp-broker bash -c "
    qpid-config add exchange headers etsi --durable -b localhost:5672 --sasl-mechanism=ANONYMOUS;
    qpid-config add exchange headers detections --durable -b localhost:5672 --sasl-mechanism=ANONYMOUS;
    qpid-config add exchange headers function_offloading --durable -b localhost:5672 --sasl-mechanism=ANONYMOUS;
    " || exit 1
  echo "AMQP 1.0 broker started."
fi
