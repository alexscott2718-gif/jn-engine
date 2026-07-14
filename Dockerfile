FROM ubuntu:24.04

WORKDIR /workspace/jn-engine
COPY . .
RUN ./scripts/bootstrap.sh

ENV LIBGL_ALWAYS_SOFTWARE=1
ENV GALLIUM_DRIVER=llvmpipe

CMD ["bash"]
