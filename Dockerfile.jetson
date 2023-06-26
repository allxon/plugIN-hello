FROM ubuntu:18.04 AS install-dependency
RUN apt-get update && apt-get install -y \
	ca-certificates \
	git \
	wget \
	build-essential \
	g++-8 \
	libssl-dev \
    gdb \
	&& rm -rf /var/lib/apt/lists/*
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 20 --slave /usr/bin/g++ g++ /usr/bin/g++-8
COPY install_cmake.sh /
RUN /bin/bash /install_cmake.sh 3.23.2

FROM install-dependency AS build-stage
ARG CMAKE_ARGS
COPY . /app
WORKDIR /app
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=release -DPLUGIN_KEY=plugin_credential.json $CMAKE_ARGS
RUN cmake --build build --target package

FROM scratch AS output-stage
ARG OUTPUT_NAME
COPY --from=build-stage /app/build/*.tar.gz /$OUTPUT_NAME