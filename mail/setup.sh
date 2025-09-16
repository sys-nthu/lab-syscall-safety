#!/usr/bin/env bash
set -euo pipefail

# This script handles the complete setup and must be run as root.
# Example: sudo bash setup.sh

# Step 1: Compile the programs
echo "Compiling the mail programs..."
# We run 'make clean' first to ensure a fresh build
make clean && make

# Step 2: Install binaries, set ownership to root, and set the SUID bit
# The `install` command is perfect for this: it copies the file and
# can set ownership (-o root) and permissions (-m 4755) in one go.
# The '4' in '4755' is what sets the SUID bit.
echo "Installing programs to /usr/local/bin and setting permissions..."
install -o root -g root -m 4755 readmail /usr/local/bin/readmail
install -o root -g root -m 4755 readmail-safe /usr/local/bin/readmail-safe

# Step 3: Create demo users
echo "👥 Creating demo users (user1, user2, user3)..."
for u in user1 user2 user3; do
  if ! id "$u" >/dev/null 2>&1; then
    useradd -m -s /bin/bash "$u"
  fi
done

# Step 4: Create the mail directory tree and per-user mailboxes
ROOTDIR=/tmp/mails
echo "📫 Creating mailboxes in $ROOTDIR..."
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

# Step 5: Create the demo password file
echo "Creating password file..."
cat > "$ROOTDIR/passwords.txt" <<'EOF'
user1:pass1
user2:pass2
user3:pass3
EOF
# Set strict permissions so only the owner can read it.
chmod 600 "$ROOTDIR/passwords.txt"

echo
echo "✅ Setup complete."
echo "You can now run 'readmail' and 'readmail-safe' from any directory."
echo "For example: readmail user1 mail1"