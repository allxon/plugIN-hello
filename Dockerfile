FROM ubuntu:20.04 AS install-dependency
RUN apt-get update && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get install -y \
	ca-certificates \
	git \
	wget \
	build-essential \
	g++ \
	libssl-dev \
    gdb \
	&& rm -rf /var/lib/apt/lists/*
COPY install_cmake.sh /
RUN /bin/bash /install_cmake.sh 3.23.2
COPY install_ninja.sh /
RUN /bin/bash /install_ninja.sh v1.11.1

FROM install-dependency AS build-stage
ARG CMAKE_ARGS
COPY . /app
WORKDIR /app
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=release -DPLUGIN_KEY=plugin_credential.json $CMAKE_ARGS
RUN cmake --build build --target package

FROM scratch AS output-stage
ARG OUTPUT_NAME
COPY --from=build-stage /app/build/*.tar.gz /$OUTPUT_NAME