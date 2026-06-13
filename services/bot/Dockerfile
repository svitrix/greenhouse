# syntax=docker/dockerfile:1.6
FROM python:3.14-slim AS builder
WORKDIR /build
COPY pyproject.toml .
RUN pip install --no-cache-dir --user .

FROM python:3.14-slim AS runtime
RUN useradd --create-home --uid 10002 bot \
    && mkdir /data && chown bot:bot /data
WORKDIR /app
COPY --from=builder /root/.local /home/bot/.local
COPY --chown=bot:bot bot ./bot
ENV PATH="/home/bot/.local/bin:${PATH}"
USER bot
# Long-polling bot — no inbound port. Persistence lives in /data (volume).
CMD ["python", "-m", "bot"]
