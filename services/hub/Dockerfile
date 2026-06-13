# syntax=docker/dockerfile:1.6
FROM python:3.14-slim AS builder
WORKDIR /build
COPY pyproject.toml .
RUN pip install --no-cache-dir --user .

FROM python:3.14-slim AS runtime
RUN useradd --create-home --uid 10001 app
WORKDIR /app
COPY --from=builder /root/.local /home/app/.local
COPY --chown=app:app app ./app
COPY --chown=app:app alembic ./alembic
COPY --chown=app:app alembic.ini ./
ENV PATH="/home/app/.local/bin:${PATH}"
USER app
EXPOSE 8000
CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000"]
