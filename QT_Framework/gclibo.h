/** @addtogroup cpp_api
  * @{
  */
  
/*! \file gclibo.h
*
* Open-source convenience functions for Galil C Lib.
* Please email softwarefeedback@galil.com with suggestions for useful/missing functions.
*
*/
#ifndef I_007AD0AF_C956_4B96_ADE2_AD04FAFFEE99
#define I_007AD0AF_C956_4B96_ADE2_AD04FAFFEE99

//set library visibility for gcc and msvc
#if BUILDING_GCLIB && HAVE_VISIBILITY
#define GCLIB_DLL_EXPORTED __attribute__((__visibility__("default")))
#elif BUILDING_GCLIB && defined _MSC_VER
#define GCLIB_DLL_EXPORTED __declspec(dllexport)
#elif defined _MSC_VER
#define GCLIB_DLL_EXPORTED __declspec(dllimport)
#else
#define GCLIB_DLL_EXPORTED
#endif

#include "gclib.h" //Galil's C Library

#ifdef _WIN32
#define GCALL __stdcall
#else
#define GCALL  /* nothing */
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MALLOCBUF G_HUGE_BUFFER //!< Malloc used for large program and array uploads.
#define MAXPROG MALLOCBUF //!< Maximum size for a program.
#define MAXARRAY MALLOCBUF //!< Maximum size for an array table upload.
#define POLLINGINTERVAL 100 //!< Interval, in milliseconds, for polling commands, e.g. GWaitForBool().
#define G_USE_GCAPS //!< Use the GCAPS server in GAddresses(), GAssign(), GIpRequests(), and GVersion(). To avoid GCAPS, comment out this line and recompile, http://galil.com/sw/pub/all/doc/gclib/html/gclibo.html

	//! Uses GUtility() and @ref G_UTIL_SLEEP to provide a blocking sleep call which can be useful for timing-based chores.
	GCLIB_DLL_EXPORTED void GCALL GSleep(unsigned int timeout_ms);
	/*!<
	*  \param timeout_ms The timeout, in milliseconds, to block before returning.
	*
	*  See GWaitForBool() for an example.
	*/


	//! Uses GUtility(), @ref G_UTIL_VERSION and @ref G_UTIL_GCAPS_VERSION to provide the library and @ref gcaps version numbers.
	GCLIB_DLL_EXPORTED GReturn GCALL GVersion(GCStringOut ver, GSize ver_len);
	/*!<
	*  \param ver Buffer to hold the output string. Buffer will be null terminated, even if the data must be truncated to do so.
	*  \param ver_len Length of buffer.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  The version number of gclib is provided first. If the @ref gcaps server can be found, 
	*  its version will be provided after a space.
	*
	*  Example with gcaps version.
	*
	*      154.190.329 1.0.0.82
	*  
	*  Example with gclib version only.
	*  
	*      154.190.329
	*  
	*  \note GVersion() will take up to 1 second to look for @ref gcaps.
	*
	*  See x_examples.cpp for an example.
	*/

	//! Uses GUtility(), @ref G_UTIL_GCAPS_ADDRESSES or @ref G_UTIL_ADDRESSES to provide a listing of all available connection addresses.
	GCLIB_DLL_EXPORTED GReturn GCALL GAddresses(GCStringOut addresses, GSize addresses_len);
	/*!<
	*  \note Serial ports are listed, e.g. COM1. Upon open, it may be necessary to specify a baud rate for the controller, e.g. `--baud 19200`.
	*        Default baud is 115200. See GOpen().
	*
	*  \param addresses Buffer to hold the output string. Buffer will be null terminated, even if the data must be truncated to do so. See below for more information.
	*  \param addresses_len Length of buffer.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  If @ref gcaps is available, the listing will come from the server via  @ref G_UTIL_GCAPS_ADDRESSES. 
	*  In the absence of the server, gclib will use @ref G_UTIL_ADDRESSES to generate the list.
	*
	*  * Ethernet controllers will be listed as *ip_address, revision_report, network_adapter_name, network_adapter_ip_address*.
	*  If an IP address is unreachable via ping, the address will be in parentheses.
	*  * PCI controllers will be listed by their identifier, e.g. GALILPCI1.
	*  * Serial ports will be listed by their identifier, e.g. COM1.
	*
	*
	*      10.1.3.91, DMC4020 Rev 1.2e, LAN, 10.1.3.10
	*      192.168.0.63, DMC4040 Rev 1.2f, Static, 192.168.0.41
	*      (192.0.0.42), RIO47102 Rev 1.1j, Static, 192.168.0.41
	*      GALILPCI1
	*      COM1
	*      COM2
	*
	*  \note GAddresses() will take up to 1 second to look for @ref gcaps.
	*
	*  See x_examples.cpp for an example.
	*/

	//! Uses GUtility() and @ref G_UTIL_INFO to provide a useful connection string.
	GCLIB_DLL_EXPORTED GReturn GCALL GInfo(GCon g, GCStringOut info, GSize info_len);
	/*!<
	*  \param g Connection's handle.
	*  \param info Buffer to hold the output string. Buffer will be null terminated, even if the data must be truncated to do so.
	*  \param info_len Length of buffer.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  The response is *address, revision_report, serial_number*. For example:
	*
	*      COM2, RIO47102 Rev 1.1j, 37290
	*
	*  See x_examples.cpp for an example.
	*/


	//! Uses GUtility() and @ref G_UTIL_TIMEOUT_OVERRIDE to set the library timeout.
	GCLIB_DLL_EXPORTED GReturn GCALL GTimeout(GCon g, short timeout_ms);
	/*!<
	*  \param g Connection's handle.
	*  \param timeout_ms The value to be used for the timeout. Use `G_USE_INITIAL_TIMEOUT` to set the timeout back to the initial GOpen() value, `--timeout`.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_gcommand.cpp and x_gread_gwrite.cpp for examples.
	*/


	//! Wrapper around GCommand for use when the return value is not desired.
	GCLIB_DLL_EXPORTED GReturn GCALL GCmd(GCon g, GCStringIn command);
	/*!<
	*  The returned data is still checked for error, e.g. `?` or timeout, but is not brought out through the prototype.
	*
	*  \param g Connection's handle.
	*  \param command Null-terminated command string to send to the controller.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_gcommand.cpp for an example.
	*/


	//! Wrapper around GCommand that trims the response.
	GCLIB_DLL_EXPORTED GReturn GCALL GCmdT(GCon g, GCStringIn command, GCStringOut trimmed_response, GSize response_len, GCStringOut* front);
	/*!<
	*  For use when the return value is desired, is ASCII (not binary), and the response should be trimmed of trailing colon, whitespace, and optionally leading space.
	*
	*  \param g Connection's handle.
	*  \param command Null-terminated command string to send to the controller.
	*  \param trimmed_response The trimmed response from the controller. Trailing space is trimmed by null terminating any trailing spaces, carriage returns, or line feeds.
	*  \param response_len The length of the trimmed_response buffer.
	*  \param front If non-null, upon return *front will point to the first non-space character in trimmed_response.
	*          This allows trimming the front of the string without modifying the user's buffer pointer, which may be allocated on the heap.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_gcommand.cpp for an example.
	*/


	//! Wrapper around GCommand that provides the return value of a command parsed into an int.
	GCLIB_DLL_EXPORTED GReturn GCALL GCmdI(GCon g, GCStringIn command, int* value);
	/*!<
	*  Use this function to get most values including TP, RP, TE, Digital I/O states, etc.
	*
	*  \param g Connection's handle.
	*  \param command Null-terminated command string to send to the controller.
	*  \param value Pointer to an int that will be filled with the return value.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_gcommand.cpp for an example.
	*/


	//! Wrapper around GCommand that provides the return value of a command parsed into a double.
	GCLIB_DLL_EXPORTED GReturn GCALL GCmdD(GCon g, GCStringIn command, double* value);
	/*!<
	*  Use this function to retrieve the full Galil 4.2 range, e.g. for a variable value with fractional data,
	*  or the value of an Analog input or Output.
	*
	*  \param g Connection's handle.
	*  \param command Null-terminated command string to send to the controller.
	*  \param value Pointer to a double that will be filled with the return value.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_gcommand.cpp for an example.
	*/

	//! Blocking call that returns when the controller evaluates the predicate as true.
	GCLIB_DLL_EXPORTED GReturn GCALL GWaitForBool(GCon g, GCStringIn predicate, int trials);
	/*!<
	*  Polls the message command (MG) to check the value of predicate.
	*  Polling will continue until the controller responds with a nonzero value
	*  or the number of polling trials is reached.
	*
	*  The amount of time until the function fails with @ref G_GCLIB_POLLING_FAILED
	*  is roughly (trials * @ref POLLINGINTERVAL) milliseconds.
	*
	*  \param g Connection's handle.
	*  \param predicate A null-terminated string containing the predicate to be polled.
	*  The predicate will be enclosed in parentheses and used in the command `MG (predicate)`
	*  to return the value.
	*  \param trials The number of polling cycles to perform looking for a nonzero value.
	*  Use -1 to poll indefinitely.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See GMotionComplete() for an example.
	*/
	
	//! Blocking call that returns once all axes specified have completed their motion.
	GCLIB_DLL_EXPORTED GReturn GCALL GMotionComplete(GCon g, GCStringIn axes);
	/*!<
	*  \note This function uses a profiled motion indicator, not the position of the encoder. E.G. see the difference between AM (profiled) and MC (encoder-based).
	*
	*  Although using the _BGm operand is the most generally compatible method,
	*  there are higher-performance ways to check for motion complete by using
	*  the data record, or interrupts. See examples x_dr_motioncomplete() and x_ei_motioncomplete().
	*
	*  \param g Connection's handle.
	*  \param axes A null-terminated string containing a multiple-axes mask. Every character in the string should be a valid argument to MG_BGm, i.e. XYZWABCDEFGHST.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_gmotioncomplete.cpp for an example.
	*/

	//! Sets the asynchronous data record to a user-specified period via `DR`.
	GCLIB_DLL_EXPORTED GReturn GCALL GRecordRate(GCon g, double period_ms);
	/*!<
	*  Takes TM and product type into account and sets the `DR` period to the period requested by the user, if possible.
	*
	*  \param g Connection's handle.
	*  \param period_ms Period, in milliseconds, to set up for the asynchronous data record.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_grecord.cpp for an example.
	*/


	//! Program download from file.
	GCLIB_DLL_EXPORTED GReturn GCALL GProgramDownloadFile(GCon g, GCStringIn file_path, GCStringIn preprocessor);
	/*!<
	*  \param g Connection's handle.
	*  \param file_path Null-terminated string containing the path to the program file.
	*  \param preprocessor Options string for preprocessing the program before sending it to the controller. See GProgramDownload().
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_programs.cpp for an example.
	*/


	//! Program upload to file.
	GCLIB_DLL_EXPORTED GReturn GCALL GProgramUploadFile(GCon g, GCStringIn file_path);
	/*!<
	*  \param g Connection's handle.
	*  \param file_path Null-terminated string containing the path to the program file, file will be overwritten if it exists.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_programs.cpp for an example.
	*/


	//! Array download from file.
	GCLIB_DLL_EXPORTED GReturn GCALL GArrayDownloadFile(GCon g, GCStringIn file_path);
	/*!<
	*  Downloads a csv file containing array data at `file_path`. If the arrays don't exist, they will be dimensioned.
	*
	*  \param g Connection's handle.
	*  \param file_path Null-terminated string containing the path to the array file.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_arrays.cpp for an example.
	*/


	//! Array upload to file.
	GCLIB_DLL_EXPORTED GReturn GCALL GArrayUploadFile(GCon g, GCStringIn file_path, GCStringIn names);
	/*!<
	*  Uploads the entire controller array table or a subset and saves the data as a csv file specified by `file_path`.
	*
	*  \param g Connection's handle.
	*  \param file_path Null-terminated string containing the path to the array file, file will be overwritten if it exists.
	*  \param names Null-terminated string containing the arrays to upload, delimited with space. "" or null uploads all arrays listed in LA.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  See x_arrays.cpp for an example.
	*/

	
	//! Uses GUtility(), @ref G_UTIL_GCAPS_IPREQUEST or @ref G_UTIL_IPREQUEST to provide a list of all Galil controllers requesting IP addresses via BOOT-P or DHCP.
	GCLIB_DLL_EXPORTED GReturn GCALL GIpRequests(GCStringOut requests, GSize requests_len);
	/*!<
	*  \param requests The buffer to hold the list of requesting controllers. Data will be null terminated, even if the data must be truncated to do so. See below for more information.
	*  \param requests_len The length of the requests buffer.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values.
	*
	*  GIpRequests() will block up to 5 seconds while listening for requests.
	*
	*  If @ref gcaps is available, the listing will come from the server via  @ref G_UTIL_GCAPS_IPREQUEST.
	*  In the absence of the server, gclib will use @ref G_UTIL_IPREQUEST to generate the list.
	*  GIpRequests() will take up to 1 second to look for @ref gcaps. 
	*  When not using @ref gcaps, Linux/OS X users must be root to use 
	*  GIpRequests() and have UDP access to bind and listen on port 67.
	*
	*  Each line of the returned data will be of the form 
	*  *model, serial_number, MAC_address, network_adapter_name, network_adapter_ip_address, remembered_ip_assignment*. 
	*  See GAssign() for more infomation about remembered IP assignments. 
	*  The following is an example output.
	*
	*      DMC2000, 34023, 00:50:4C:00:84:E7, enp5s0, 192.168.42.92, 192.168.42.200
	*      DMC2105, 7, 00:50:4C:58:00:07, enp5s0, 192.168.42.92, 0.0.0.0
	*      DMC2105, 13, 00:50:4C:58:00:0D, enp5s0, 192.168.42.92, 0.0.0.0
	*
	*  See x_examples.cpp for an example.
	*/

	//! Uses GUtility(), @ref G_UTIL_GCAPS_SET_SERVER to set the new active server
	GCLIB_DLL_EXPORTED GReturn GCALL GSetServer(GCStringIn server_name);
	/*!< 
	*  \note This function is only available on Windows 10 and Linux.
	*
	*  \param server_name The name of the server to set as your new active server.
	*
	*  Use this function in conjunction with @ref GListServers().  Choose a name received from GListServers()
	*  to set as your new active server.
	*
	*  After setting a new active server, all gclib calls will route through that new active server, unless explicitly noted otherwise.
	*
	*  To set your active server back to your local server, simply pass "Local" to GSetServer():  
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values. 
	*
	*/

	//! Uses GUtility(), @ref G_UTIL_GCAPS_LIST_SERVERS to provide a list of all available gcaps services on the local network.
	GCLIB_DLL_EXPORTED GReturn GCALL GListServers(GCStringOut servers, GSize servers_len);
	/*!< 
	*  \note This function is only available on Windows 10 and Linux.
	*
	*  \param servers The buffer to hold the list of available gcaps servers
	*  \param servers_len The length of the servers buffer
	*
	*  This function is used to find a list of available gcaps servers that have made themselves "Discoverable".
	*
	*  The list of available servers are separated by a newline '\\n' character.
	*
	*  \attention This function will always use your local gcaps server, regardless of which server you have set as your active
	*	server.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values. 
	*
	*/

	//! Uses GUtility(), @ref G_UTIL_GCAPS_PUBLISH_SERVER to publish local gcaps server to the local network.
	GCLIB_DLL_EXPORTED GReturn GCALL GPublishServer(GCStringIn name, GOption publish, GOption save);
	/*!< 
	*  \note This function is only available on Windows 10 and Linux.
	*
	*  \param name The name of the server to publish or remove
	*  \param publish Option to publish or remove server from network
	*  \param save Option to save this configuration for future reboots
	*
	*  This function is used to make your local gcaps server "Discoverable" or "Invisible"
	*
	*  publish Option:<br> 
	*  Set to 1 to publish server to the network and make "Discoverable"<br>
	*  Set to 0 to remove server from the network and make "Invisible"				
	*
	*  save Option: <br>
	*  Set to 1 to save the configuration for future reboots of the server<br>
	*  Set to 0 to use this configuration once, and not overwrite previous server settings
	*
	*  \attention This function will always use your local gcaps server, regardless of which server you have set as your active
	*	server.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values. 
	*
	*/

	//! Uses GUtility(), @ref G_UTIL_GCAPS_SERVER_STATUS to get information on the local server name and if it is published to the local network.
	GCLIB_DLL_EXPORTED GReturn GCALL GServerStatus(GCStringOut status, GSize status_len);
	/*!< 
	*  \note This function is only available on Windows 10 and Linux.
	*
	*  \param status The buffer to hold the status of the local gcaps server
	*  \param status_len The length of the status buffer
	*
	*  This function is used to find the status of your local gcaps server.  Use this function 
	*  to determine the name your server is currently using, and whether or not your gcaps server is currently set to "Discoverable" or "Invisible"
	*
	*  The status buffer will be filled  in the form of "[Server Name], [Discoverable]"
	*
	*  For example, for a server with the name "Example Server" that is set to "Discoverable", the status buffer would contain "Example Server, true".
	*
	*  \attention This function will always use your local gcaps server, regardless of which server you have set as your active
	*	server.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values. 
	*
	*/


	//! Uses GUtility(), @ref G_UTIL_GCAPS_REMOTE_CONNECTIONS to get a list of remote addresses connected to the local server.
	GCLIB_DLL_EXPORTED GReturn GCALL GRemoteConnections(GCStringOut connections, GSize connections_length);
	/*!< 
	*  \note This function is only available on Windows 10 and Linux.
	*
	*  \param connections The buffer to hold the list of remote IP addresses currently connected to your hardware
	*  \param connections_len The length of the connections buffer
	*
	*  This function is used to find a list of IP Addresses of machines that currently have open connections to your local hardware. 
	*  If another user sets your local server as their active server, and then opens a connection to your hardware, their IP Address will appear in this
	*  list.
	*
	*  The list of IP addresses are separated by a newline '\\n' character.
	*
	*
	*  \attention This function will always use your local gcaps server, regardless of which server you have set as your active
	*	server.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values. 
	*
	*/

	//! Uses GUtility(), @ref G_UTIL_GCAPS_ASSIGN or @ref G_UTIL_ASSIGN to assign an IP address over the Ethernet to a controller at a given MAC address.
	GCLIB_DLL_EXPORTED GReturn GCALL GAssign(GCStringIn ip, GCStringIn mac);
	/*!< 
	*  \param ip The null-terminated ip address to assign. The hardware should not yet have an IP address.
	*  \param mac The null-terminated MAC address of the hardware.
	*
	*  \return The success status or error code of the function. See gclib_errors.h for possible values. 
	*
	*  On Linux and Mac, the desired IP address will be pinged prior to the assignment. If the ping is returned, GAssign() will return @ref G_GCLIB_UTILITY_IP_TAKEN.
	*
	*  If @ref gcaps is available, the assign will be performed from the server 
    *  via @ref G_UTIL_GCAPS_ASSIGN. @ref gcaps will remember the assignment and
    *  will automatically assign the desired IP address should the controller 
    *  ever request one again, e.g. after a controller master reset. To clear 
    *  the remembered IP address from gcaps, call GAssign() with a blank string
    *  in place of the ip address. To remove all remembered ip addresses, 
    *  specfify a blank string for the mac address.
    *
    *  In the absence of the server, gclib will use @ref G_UTIL_ASSIGN to assign.
    *  GAssign() will take up to 1 second to look for @ref gcaps.
	*  When not using @ref gcaps, Linux/OS X users must be root to use GAssign() and have UDP access to send on port 68.
	*
	*  See x_examples.cpp for an example.
	*/
		
	//! Provides a human-readable description string for return codes.
	GCLIB_DLL_EXPORTED void GCALL GError(GReturn rc, GCStringOut error, GSize error_len);
	/*!<
	*  \param rc The return code to lookup.
	*  \param error The buffer to fill with the error text. Buffer will be null terminated, even if the data must be truncated to do so.
	*  \param error_len The length of the error buffer.
	*
	*  See x_examples.cpp for an example.
	*/

#ifndef G_OMIT_GSETUPDDOWNLOADFILE
	//! Download a saved controller configuration from a file.
	GCLIB_DLL_EXPORTED GReturn GCALL GSetupDownloadFile(GCon g, GCStringIn file_path, GOption options, GCStringOut info, GSize info_len);
	/*!<
	*  \param g Connection's handle.
	*  \param file_path Null-terminated string containing the path to the gcb file.
	*  \param options Bit mask to determine what configuration data to download. See below for all options.
	*  \param info Optional pointer to a buffer to store the controller info. If no info is needed, specify as NULL.
	*  \param info_len Length of optional info buffer. If no info is needed, specify as NULL.
	*
	*  \return The success status or error code of the function. 
	*  If the options parameter is set to 0, the return value will be a bit mask indicating which sectors in the specified GCB are not empty. 
	*  Otherwise, see gclib_errors.h for possible error values.
	*
	*  \note By default, GSetupDownloadFile() will stop immediately if an error is encountered downloading data.
	*  This can be overridden in the options parameter. 
	*  For example, you may want to override the error if you have a backup from an 8-axis controller 
	*  and want to restore the parameters for the first 4 axes to a 4-axis controller.
	*
	*  If both info and info_len are not NULL, the controller information will be provided regardless of the options parameter.
	*
	*  The options parameter is a bit mask. If options is set to 0, 
	*  GSetupDownloadFile() will return a bit mask indicating which sectors in the specified GCB are not empty. 
	*  The following contains a list of all currently available options:
	*
	*  Bit |  Value  | Function | Description
	*  :-: |  -----  | -------- | -----------
	*   1  |  0x0002 | Restore parameters | \b KPA, \b KIA, \b KDA, etc...
	*   3  |  0x0008 | Restore variables | Variables are listed by the \b LV command
	*   4  |  0x0010 | Restore arrays | Arrays are listed by the \b LA command
	*   5  |  0x0020 | Restore program | The program is listed by the \b LS command
	*   31 |  0x8000 | Ignore errors | Ignore invalid parameter errors and continue restoring data. GSetupDownloadFile() will still stop immediately if a connection issue or other fatal error is encountered
	*
	*  Usage example:
	*  \code
	*  GCon g;
	*  GOption opt = 0;
	*
	*  GCStringOut info;
	*  GSize info_len = 4096;
	*
	*  GReturn rc = GOpen("192.168.0.50", &g);
	*  if (rc) return rc;
	*
	*  // Call GSetupDownloadFile() with options set to 0 so we can get the non-empty sector bit mask
	*  opt = GSetupDownloadFile(g, "C:\\path\\to\\gcb\\file.gcb", 0, NULL, NULL);
	*
	*  info = (GCStringOut)malloc(sizeof(GCStringOut) * info_len);
	*
	*  // Call GSetupDownloadFile() with the bit mask returned in the previous function call
	*  rc = GSetupDownloadFile(g, "C:\\path\\to\\gcb\\file.gcb", opt, info, info_len);
	*
	*  printf("Info:\n\n%s", info);
	*
	*  GClose(g);
	*
	*  free(info);
	*  return rc;
	*  \endcode
	*/
#endif //G_OMIT_GSETUPDDOWNLOADFILE

#ifdef GCLIB_LOGGING
	void LogMsg(const char* msg);
#endif

#ifdef __cplusplus
} //extern "C"
#endif

#endif //I_007AD0AF_C956_4B96_ADE2_AD04FAFFEE99
/** @}*/
