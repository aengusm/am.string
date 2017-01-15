# am.string~
A Karplus-Strong based string synthesizer object for Cycling '74 Max.

The am.string~ object is based on Sullivan's implementation [1] of the Karplus-Strong algorithm for plucked string synthesis [2] though Sullivan's distortion and feedback components are not included. However, instead of using simple linear interpolation to set delay times corresponding to non-integer numbers of samples, it uses a 7th-order Lagrange filter [3] to perform the interpolation. This reduces the low frequency roll-off associated with delay times close to n+0.5 samples. In addition, it is implemented so that any signal can be passed through the 'string' to achieve a variety of resonant filter effects.

The sound output of the am.string~ object is not independent of the sample rate used. This is because of the low-pass filter that determines the timbre: The 'gain' parameter sets the feedback gain at the fundamental frequency and the 'brightness' parameter sets the feedback gain at the Nyquist frequency (relative to that at the fundamental). Thus, the output depends on the sample rate used: keeping the parameters constant, the sound will be very slightly different at different sample rates.

This implementation prioritises robustness over performance. For example there is protection on the gain and period inputs (the middle and righthand signal inputs) so that inappropriate values (that would cause the output signal to explode) will not be accepted. This comes at some CPU cost, but it makes the object much safer to play with.

## References

[1] Sullivan, C. R. (1990). Extending the Karplus-Strong algorithm to synthesize electric guitar timbres with distortion and feedback. Computer Music Journal, 14(3), 26–37.

[2] Karplus, K., & Strong, A. (1983). Digital Synthesis of Plucked-String and Drum Timbres. Computer Music Journal, 7(2), 43–55.

[3] Laakso, T. I., Valimaki, V., Karjalainen, M., & Laine, U. (1996). Splitting the unit delay: Tools for Fractional Delay Filter Design. Signal Processing Magazine, IEEE, 13(1), 30–60.
