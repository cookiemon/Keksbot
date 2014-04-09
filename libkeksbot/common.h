#ifndef COMMON_H
#define COMMON_H

#if defined _WIN32 || defined __CYGWIN__
	#ifdef KEKSBOT_EXPORTS
		#ifdef __GNUC__
			#define KEKSBOT_API __attribute__((dllexport))
		#else
			#define KEKSBOT_API __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define KEKSBOT_API __attribute__((dllimport))
		#else
			#define KEKSBOT_API __attribute__(dllimport)
		#endif
	#endif
#else
	#if __GNUC__ >= 4
		#define KEKSBOT_API __attribute__((visibility("default")))
	#else
		#define KEKSBOT_API
	#endif
#endif

#endif

