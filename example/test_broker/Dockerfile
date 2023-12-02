FROM ubuntu:20.04

ENV DEBIAN_FRONTEND="noninteractive" \
    TERM="xterm"
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
RUN apt-get -y update && apt-get install -y \
      git \
      build-essential \
      python3 \
      python3-distutils \
      python3-setuptools \
      python3-dev \
      ruby \
      cmake \
      libboost-dev \
      libboost-program-options-dev \
      libboost-system-dev \
      libboost-test-dev \
      uuid-dev \
      libnss3-dev \
      libnss3-tools \
      libsasl2-dev \
      sasl2-bin \
      swig \
      python-dev \
      python-setuptools \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN cd /tmp/ && \
  git clone https://github.com/apache/qpid-proton.git && \
  cd qpid-proton && \
  git checkout 0.34.0 && \
  mkdir build && \
  cd build && \
  cmake -DCMAKE_BUILD_TYPE=Release  -DSYSINSTALL_BINDINGS=ON .. && \
  make -j$(nproc --all) install && \
  cd /tmp/ && \
  rm -rf qpid-proton

# Commits newer than d0bf9e36f31b2e753c2ac855dddc6de4024b40f6 do not compile due to libnspr being too recent
# qpid-cpp build tests only run on ubuntu 16.04, so maybe 18.04/20.04 are too new and do not work with the most recent qpid-cpp version
# fixing would probably not be too hard, but I'm not in the mood for a pull request
RUN cd /tmp/ && \
  git clone https://github.com/apache/qpid-cpp.git && \
  cd qpid-cpp && \
  git checkout d0bf9e36f31b2e753c2ac855dddc6de4024b40f6 && \
  mkdir build && \
  cd build && \
  cmake -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE="$(which python2)" .. && \
  make -j$(nproc --all) install && \
  cd /tmp/ && \
  rm -rf qpid-cpp

# Add qpidd group / user
RUN groupadd -r qpidd && useradd -r -d /var/lib/qpidd -m -g qpidd qpidd
RUN chown -R qpidd:qpidd /var/lib/qpidd
VOLUME /var/lib/qpidd
USER qpidd:qpidd

ADD ./broker_entrypoint.sh /usr/bin/
ENTRYPOINT ["/usr/bin/broker_entrypoint.sh"]

EXPOSE 5672
CMD ["qpidd"]
