cmake_minimum_required(VERSION 3.21)

project(
	PSX-test
	LANGUAGES    C ASM
	DESCRIPTION  "Blaxar's PSn00bSDK test"
)

include(GNUInstallDirs)

add_subdirectory(src)
