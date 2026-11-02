#!/bin/bash
# Back up the data that lives on the tablet's userdata partition.
#
#   ./scripts/backup-tablet.sh [destination]
#
# The default destination is backups/<date>-<time> on the host. The tarball
# contains the user's home, /etc (Wi-Fi passwords, SSH keys, service
# configuration), Bluetooth pairings and the accounts database. It is
# chmod 600 because of the secrets.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
KEY=$ROOT/build/ssh/xun_key
TABLET=${TABLET:-root@172.16.42.1}
DEST=${1:-$ROOT/backups/$(date +%Y%m%d-%H%M%S)}
SSH="ssh -i $KEY -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR"

mkdir -p "$DEST"

$SSH "$TABLET" 'tar -C / -czf - home/xun etc var/lib/bluetooth var/lib/AccountsService root/.ssh' \
	>"$DEST/system.tar.gz"
chmod 600 "$DEST/system.tar.gz"

$SSH "$TABLET" 'apk info -v 2>/dev/null' >"$DEST/packages.txt"
$SSH "$TABLET" 'cat /etc/apk/world 2>/dev/null' >"$DEST/world.txt"
{
	$SSH "$TABLET" 'date; uname -a; cat /etc/hostname'
	echo
	echo "== partitions"
	$SSH "$TABLET" 'lsblk -o NAME,SIZE,FSTYPE,LABEL,PARTLABEL 2>/dev/null'
} >"$DEST/system-info.txt"

du -sh "$DEST"
echo "backup written to $DEST"
