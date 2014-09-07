/* Python on Windows unfortunately wraps all 64 bit Windows defines
 * in #ifdef _MSC_VER. In case of GCC, it only checks for 32 bit.
 *
 * Therefore, define MS_WIN64 (used by the Python headers) if we
 * actually are on 64 bit Windows (_WIN64)
 */

#if defined(_WIN64) && !defined(MS_WIN64)
#define MS_WIN64 1
#endif
