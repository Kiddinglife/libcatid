# Overview #

LibCat is split up into several modular components so that only the interesting subset of features may be selected for use with little to no modification:

  * **LibCatCommon** includes basic, shared features used by the rest of LibCat.

  * **LibCatCodec** provides compression.

  * **LibCatMath** provides assembly-optimized large unsigned integer arithmetic, arithmetic in a finite field with a pseudo-Mersenne modulus, and arithmetic on Twisted Edwards elliptic curves.

  * **LibCatCrypt** provides implementations of modern cryptographic primitives.

  * **LibCatTunnel** provides fast, secure Key Agreement and Authenticated Encryption for networking.

  * **LibCatFramework** provides a scalable, multithreaded framework for application development.