#!/bin/bash
source ./scripts/show/divbar
echo 'Uninstalling retroserver...'
sleep .5

systemctl stop retroserver.service
systemctl disable retroserver.service
rm -f /etc/systemd/system/retroserver.service
systemctl daemon-reload

rm -rf /usr/local/bin/retroserver

source ./scripts/show/divbar
echo 'Done. Files in /srv/retroserver were NOT deleted.'
sleep .5
