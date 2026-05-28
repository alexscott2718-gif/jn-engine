# Local environment configuration

Host addresses, auth tokens, and VNC passwords are **never** committed to
this repo. Instrumentation tooling reads them from environment variables at
runtime, and the on-XP proxy bakes its receiver endpoint in at build time
through compiler defines.

## Required variables

| Variable                 | Used by                                    | Notes                                        |
|--------------------------|--------------------------------------------|----------------------------------------------|
| `XP_HOST`                | `instrument/deploy_xp.py`                  | XP machine LAN address                       |
| `XP_PORT`                | `instrument/deploy_xp.py` (default `9999`) | XP command-server port                       |
| `XP_TOKEN`               | `instrument/deploy_xp.py`                  | XP command-server auth token (rotate freely) |
| `OMTC_RECEIVER_ENDPOINT` | `instrument/deploy_xp.py`                  | `host:port` the receiver listens on          |
| `OMTC_RECEIVER_IP`       | `instrument/proxy/build.sh`                | Compiled into `ddraw.dll` as `-D`            |
| `OMTC_RECEIVER_PORT`     | `instrument/proxy/build.sh`                | Compiled into `ddraw.dll` as `-D`            |
| `VNC_HOST`               | `tools/vnccap.py`                          | VNC server address                           |
| `VNC_PORT`               | `tools/vnccap.py` (default `5900`)         |                                              |
| `VNC_PASSWORD`           | `tools/vnccap.py`                          | Or pass as the second positional arg         |

## Recommended setup

Drop the values into a `.env` (gitignored) and source it before working:

```bash
export XP_HOST=10.x.x.x
export XP_TOKEN=...
export OMTC_RECEIVER_ENDPOINT=10.x.x.x:7070
export OMTC_RECEIVER_IP=10.x.x.x
export VNC_HOST=10.x.x.x
export VNC_PASSWORD=...
```

## Rebuilding the proxy after changing the receiver

```bash
OMTC_RECEIVER_IP=10.x.x.x OMTC_RECEIVER_PORT=7070 \
    bash instrument/proxy/build.sh
```

The built `ddraw.dll` is **not** tracked — it is regenerated locally and
embeds the receiver host address as a string constant.
