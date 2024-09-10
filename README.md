# 1️⃣🐝🏎️ The One Billion Row Challenge
The challenge: compute simple floating-point math over 1 billion rows. As fast as possible, without dependencies.

I did basic implemantation on one thread(and still goes ~800MB/s) using simple hash map and computation directly from memory mapped file.
My record is 17.4s on ryzen 5800X.
