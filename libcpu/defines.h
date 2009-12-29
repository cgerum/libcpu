/*
 * Add big switches for testing here.
 * libcpu shouldn't use many ifdefs. Consider dropping one of
 * the alteratives or make it a runtime option (i.e. let the
 * client decide).
 */

// Copy register set paramters into a local array
// this hints LLVM to not care about writing back
// the contents too often.
//#define OPT_LOCAL_REGISTERS

// Limits the DFS when tagging code, so that we don't
// translate all reachable code at a time, but only a
// certain amount of code in advance, and translate more
// on demand. 6 is the optimum for OpenBSD's 'date' on M88K.
// If this is turned off, we do "entry caching", i.e. we
// create a file in /tmp that holds all entries to the code
// (i.e. all start addresses that can't be found automatically),
// and we start tagging at these addresses on load if the
// cache exists.
//#define LIMIT_TAGGING_DFS 6
