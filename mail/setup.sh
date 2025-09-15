#!/usr/bin/env bash
set -euo pipefail

# Run as root inside the devcontainer:
#   sudo bash setup.sh

# 1) Create demo users 
for u in user1 user2 user3; do
  if ! id "$u" >/dev/null 2>&1; then
    useradd -m -s /bin/bash "$u"
  fi
done

# 2) Create $ROOTDIR tree and per-user mailboxes
ROOTDIR=/tmp/mails
mkdir -p "$ROOTDIR"
for u in user1 user2 user3; do
  mkdir -p "$ROOTDIR/$u"
  chown "$u:$u" "$ROOTDIR/$u"
  chmod 750 "$ROOTDIR/$u"

  for i in 1 2 3; do
    cat > "$ROOTDIR/$u/mail$i" <<EOF
From: admin@example.com
To: $u@example.com
Subject: Welcome, $u! (Mail $i)

This is a demo mail body for $u.
EOF
    chown "$u:$u" "$ROOTDIR/$u/mail$i"
    chmod 600 "$ROOTDIR/$u/mail$i"
  done
done

# 3) Demo password file (plain text on purpose for the lab)
cat > "$ROOTDIR/passwords.txt" <<'EOF'
user1:pass1
user2:pass2
user3:pass3
EOF
chmod 600 "$ROOTDIR/passwords.txt"

echo "Users: user1/user2/user3 (passwords in $ROOTDIR/passwords.txt)."
echo "Mail dirs: $ROOTDIR/{user1,user2,user3} with per-user 'mail' file."