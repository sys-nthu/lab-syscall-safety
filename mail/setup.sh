#!/usr/bin/env bash
set -euo pipefail

# Run as root inside the devcontainer:
#   sudo bash setup.sh

# 1) Create demo users (no home dir, no login shell)
for u in user1 user2 user3; do
  if ! id "$u" >/dev/null 2>&1; then
    useradd -M -s /usr/sbin/nologin "$u"
  fi
done

# 2) Create mails tree and per-user mailboxes
mkdir -p mails
for u in user1 user2 user3; do
  mkdir -p "mails/$u"
  chown "$u:$u" "mails/$u"
  chmod 750 "mails/$u"

  for i in 1 2 3; do
    cat > "mails/$u/mail$i" <<EOF
From: admin@example.com
To: $u@example.com
Subject: Welcome, $u! (Mail $i)

This is a demo mail body for $u.
EOF
    chown "$u:$u" "mails/$u/mail$i"
    chmod 600 "mails/$u/mail$i"
  done
done

# 3) Demo password file (plain text on purpose for the lab)
cat > passwords.txt <<'EOF'
user1:pass1
user2:pass2
user3:pass3
EOF
chmod 600 passwords.txt

echo "Users: user1/user2/user3 (passwords in passwords.txt)."
echo "Mail dirs: ./mails/{user1,user2,user3} with per-user 'mail' file."