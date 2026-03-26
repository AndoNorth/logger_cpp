"""
E2E tests for /config/logger endpoints.

Covers:
  GET /config/logger - Get logging configuration
  PUT /config/logger - Update logging configuration
"""

import pytest

from conftest import url_for

pytestmark = [pytest.mark.config, pytest.mark.usefixtures("mss_is_running")]


class TestLoggerConfig:
    """GET/PUT /config/logger"""

    def test_get_logger_config(self, api):
        # GIVEN: A running MSS server

        # WHEN: We request the logger configuration
        resp = api.get(url_for(api, "/config/logger"))

        # THEN: The response is a JSON object
        assert resp.status_code == 200
        data = resp.json()
        assert isinstance(data, dict)

    def test_update_logger_config_roundtrip(self, api):
        # GIVEN: The current logger configuration
        resp = api.get(url_for(api, "/config/logger"))
        assert resp.status_code == 200
        original = resp.json()

        # WHEN: We write it back unchanged
        resp = api.put(url_for(api, "/config/logger"), json=original)

        # THEN: The update succeeds without error
        assert resp.status_code == 200
