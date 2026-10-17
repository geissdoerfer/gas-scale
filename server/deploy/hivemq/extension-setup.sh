#!/bin/sh
# Disables the hivemq-allow-all-extension (enabled by default in the image -
# accepts any client with no credentials) and enables the bundled
# Enterprise Security Extension (disabled by default), which is configured
# via ./ese/config.xml + ./ese/ese-file-realm.xml to require the shared
# MQTT username/password. Extensions are toggled by the presence/absence of
# a DISABLED marker file in their directory - confirmed directly against
# the image's shipped state, not assumed.
set -e

touch /opt/hivemq/extensions/hivemq-allow-all-extension/DISABLED
rm -f /opt/hivemq/extensions/hivemq-enterprise-security-extension/DISABLED

exec /opt/docker-entrypoint.sh "$@"
