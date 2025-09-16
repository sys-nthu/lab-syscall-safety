
# Lab: SUID Root is Dangerous



**Initialize the Environment**: The `setup.sh` script creates two users (`user1`, `user2`) and sets up their password and mail files for our experiment.

    ```bash
    # Run the setup script with root privileges
    sudo bash setup.sh
    ```



### Normal Usage

```bash
# The first argument is the username, the second is the mail file.
sudo ./readmail user1 mail1
Password: pass1
# Expected output: The content of user1's mail1.
```

### The Attack

Now, let's try something malicious. We will authenticate as `user1`, but ask the program to read a file at the path `../user2/mail1`. The `..` tells the system to go up one directory level.

```bash
sudo ./readmail user1 ../user2/mail1
Password: pass1
# Success! We can now read the contents of user2's private mail. 😱
```



###  Attempt the Same Attack

We'll repeat the exact same attack, this time targeting `readmail-safe`.

```bash
sudo ./readmail-safe user1 ../user2/mail1
Password: pass1
# Expected output: "Permission denied" or a similar error. The attack fails! 😎
```


### Clean Up


```bash
# Clean up the users and temporary files
sudo userdel user1 || true
sudo userdel user2 || true
sudo rm -rf /tmp/mails /tmp/passwords.txt
```
