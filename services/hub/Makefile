.PHONY: dev bot-logs test up down logs psql migrate fmt smoke ui ui-logs

dev:
	uvicorn app.main:app --reload

bot-logs:
	docker compose logs -f bot

test:
	pytest -v

up:
	docker compose up -d --build

down:
	docker compose down

logs:
	docker compose logs -f hub

psql:
	docker compose exec timescaledb psql -U postgres -d greenhouse

migrate:
	alembic upgrade head

fmt:
	ruff check --fix .

ui:
	docker compose build dashboard
	docker compose up -d dashboard

ui-logs:
	docker compose logs -f dashboard

smoke:
	docker compose up -d --build
	@echo "Waiting for hub..."
	@until curl -s http://localhost:8000/ >/dev/null 2>&1; do sleep 1; done
	docker compose exec hub alembic upgrade head
	@echo ""
	@echo "Step 1/3: Mint an admin token"
	@ADMIN_TOKEN=$$(docker compose exec hub python -m app.tools.mint_admin_token --name smoke 2>/dev/null | awk '/admin_token:/ {print $$2}'); \
	  echo "  admin_token: $$ADMIN_TOKEN"; \
	  echo "Step 2/3: Open a pairing window"; \
	  CODE=$$(curl -s -X POST http://localhost:8000/api/pairing/open \
	    -H "Authorization: Bearer $$ADMIN_TOKEN" \
	    -H 'Content-Type: application/json' \
	    -d '{"ttl_seconds": 300}' | jq -r .code); \
	  echo "  pairing_code: $$CODE"; \
	  echo "Step 3/3: Claim the code as device gh-smoke"; \
	  API_KEY=$$(curl -s -X POST http://localhost:8000/api/pairing/claim \
	    -H 'Content-Type: application/json' \
	    -d "{\"claim_code\":\"$$CODE\",\"device_id\":\"gh-smoke\",\"mac\":\"aa:bb:cc:dd:ee:ff\",\"fw_version\":\"0.4.0\",\"profile_id\":\"gh-coordinator-v1\"}" | jq -r .api_key); \
	  echo "  api_key: $$API_KEY"; \
	  echo ""; \
	  echo "Posting an air_temp reading to /ingest..."; \
	  curl -s -X POST http://localhost:8000/ingest \
	    -H "Authorization: Bearer $$API_KEY" \
	    -H 'Content-Type: application/json' \
	    -d "{\"device_id\":\"gh-smoke\",\"fw_version\":\"0.4.0\",\"batch_id\":\"smoke-01\",\"readings\":[{\"ts\":$$(date +%s)000,\"channel_id\":0,\"kind\":\"air_temp\",\"value\":21.5,\"status\":0}],\"events\":[]}" | jq
	@echo ""
	@echo "Verify the row landed:"
	@echo "  make psql"
	@echo "  SELECT count(*), max(ts) FROM readings WHERE device_id='gh-smoke';"
