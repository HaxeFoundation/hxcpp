#ifdef _MSC_VER



#if _MSC_VER>=1800 && _MSC_VER<1900

#include <stdexcept>
#include <cstdlib>
#include <system_error>
#pragma warning(disable: 4005)
#include <windows.h>

namespace std
{

_CRTIMP2_PURE _NO_RETURN(__CLRCALL_PURE_OR_CDECL _Xbad_alloc())
	{	// report a bad_alloc error
	_THROW_NCEE(_XSTD bad_alloc, _EMPTY_ARGUMENT);

	}

_CRTIMP2_PURE _NO_RETURN(__CLRCALL_PURE_OR_CDECL _Xinvalid_argument(_In_z_ const char *_Message))
	{	// report an invalid_argument error
	_THROW_NCEE(invalid_argument, _Message);
	}

_CRTIMP2_PURE _NO_RETURN(__CLRCALL_PURE_OR_CDECL _Xlength_error(_In_z_ const char *_Message))
	{	// report a length_error
	_THROW_NCEE(length_error, _Message);
	}

_CRTIMP2_PURE _NO_RETURN(__CLRCALL_PURE_OR_CDECL _Xout_of_range(_In_z_ const char *_Message))
	{	// report an out_of_range error
	_THROW_NCEE(out_of_range, _Message);
	}

_CRTIMP2_PURE _NO_RETURN(__CLRCALL_PURE_OR_CDECL _Xoverflow_error(_In_z_ const char *_Message))
	{	// report an overflow error
	_THROW_NCEE(overflow_error, _Message);
	}

_CRTIMP2_PURE _NO_RETURN(__CLRCALL_PURE_OR_CDECL _Xruntime_error(_In_z_ const char *_Message))
	{	// report a runtime_error
	_THROW_NCEE(runtime_error, _Message);
	}




struct _Win_errtab_t
	{	// maps Windows error_code to NTBS
	int _Errcode;
	const char *_Name;
	};

static const _Win_errtab_t _Win_errtab[] =
	{	// table of Windows code/name pairs
	ERROR_ACCESS_DENIED, "permission denied",
	ERROR_ALREADY_EXISTS, "file exists",
	ERROR_BAD_UNIT, "no such device",
	ERROR_BUFFER_OVERFLOW, "filename too long",
	ERROR_BUSY, "device or resource busy",
	ERROR_BUSY_DRIVE, "device or resource busy",
	ERROR_CANNOT_MAKE, "permission denied",
	ERROR_CANTOPEN, "io error",
	ERROR_CANTREAD, "io error",
	ERROR_CANTWRITE, "io error",
	ERROR_CURRENT_DIRECTORY, "permission denied",
	ERROR_DEV_NOT_EXIST, "no such device",
	ERROR_DEVICE_IN_USE, "device or resource busy",
	ERROR_DIR_NOT_EMPTY, "directory not empty",
	ERROR_DIRECTORY, "invalid argument",
	ERROR_DISK_FULL, "no space on device",
	ERROR_FILE_EXISTS, "file exists",
	ERROR_FILE_NOT_FOUND, "no such file or directory",
	ERROR_HANDLE_DISK_FULL, "no space on device",
	ERROR_INVALID_ACCESS, "permission denied",
	ERROR_INVALID_DRIVE, "no such device",
	ERROR_INVALID_FUNCTION, "function not supported",
	ERROR_INVALID_HANDLE, "invalid argument",
	ERROR_INVALID_NAME, "invalid argument",
	ERROR_LOCK_VIOLATION, "no lock available",
	ERROR_LOCKED, "no lock available",
	ERROR_NEGATIVE_SEEK, "invalid argument",
	ERROR_NOACCESS, "permission denied",
	ERROR_NOT_ENOUGH_MEMORY, "not enough memory",
	ERROR_NOT_READY, "resource unavailable try again",
	ERROR_NOT_SAME_DEVICE, "cross device link",
	ERROR_OPEN_FAILED, "io error",
	ERROR_OPEN_FILES, "device or resource busy",
	ERROR_OPERATION_ABORTED, "operation canceled",
	ERROR_OUTOFMEMORY, "not enough memory",
	ERROR_PATH_NOT_FOUND, "no such file or directory",
	ERROR_READ_FAULT, "io error",
	ERROR_RETRY, "resource unavailable try again",
	ERROR_SEEK, "io error",
	ERROR_SHARING_VIOLATION, "permission denied",
	ERROR_TOO_MANY_OPEN_FILES, "too many files open",
	ERROR_WRITE_FAULT, "io error",
	ERROR_WRITE_PROTECT, "permission denied",
	WSAEACCES, "permission_denied",
	WSAEADDRINUSE, "address_in_use",
	WSAEADDRNOTAVAIL, "address_not_available",
	WSAEAFNOSUPPORT, "address_family_not_supported",
	WSAEALREADY, "connection_already_in_progress",
	WSAEBADF, "bad_file_descriptor",
	WSAECONNABORTED, "connection_aborted",
	WSAECONNREFUSED, "connection_refused",
	WSAECONNRESET, "connection_reset",
	WSAEDESTADDRREQ, "destination_address_required",
	WSAEFAULT, "bad_address",
	WSAEHOSTUNREACH, "host_unreachable",
	WSAEINPROGRESS, "operation_in_progress",
	WSAEINTR, "interrupted",
	WSAEINVAL, "invalid_argument",
	WSAEISCONN, "already_connected",
	WSAEMFILE, "too_many_files_open",
	WSAEMSGSIZE, "message_size",
	WSAENAMETOOLONG, "filename_too_long",
	WSAENETDOWN, "network_down",
	WSAENETRESET, "network_reset",
	WSAENETUNREACH, "network_unreachable",
	WSAENOBUFS, "no_buffer_space",
	WSAENOPROTOOPT, "no_protocol_option",
	WSAENOTCONN, "not_connected",
	WSAENOTSOCK, "not_a_socket",
	WSAEOPNOTSUPP, "operation_not_supported",
	WSAEPROTONOSUPPORT, "protocol_not_supported",
	WSAEPROTOTYPE, "wrong_protocol_type",
	WSAETIMEDOUT, "timed_out",
	WSAEWOULDBLOCK, "operation_would_block",
	0, 0
	};

const char *_Winerror_map(int _Errcode)
	{	// convert to name of Windows error
	const _Win_errtab_t *_Ptr = &_Win_errtab[0];
	for (; _Ptr->_Name != 0; ++_Ptr)
		if ((int)_Ptr->_Errcode == _Errcode)
			return (_Ptr->_Name);
	return (0);
	}

struct _Sys_errtab_t
	{	// maps error_code to NTBS
	generic_errno _Errcode;
	const char *_Name;
	};

static const _Sys_errtab_t _Sys_errtab[] =
	{	// table of Posix code/name pairs
	errc::address_family_not_supported, "address family not supported",
	errc::address_in_use, "address in use",
	errc::address_not_available, "address not available",
	errc::already_connected, "already connected",
	errc::argument_list_too_long, "argument list too long",
	errc::argument_out_of_domain, "argument out of domain",
	errc::bad_address, "bad address",
	errc::bad_file_descriptor, "bad file descriptor",
	errc::bad_message, "bad message",
	errc::broken_pipe, "broken pipe",
	errc::connection_aborted, "connection aborted",
	errc::connection_already_in_progress, "connection already in progress",
	errc::connection_refused, "connection refused",
	errc::connection_reset, "connection reset",
	errc::cross_device_link, "cross device link",
	errc::destination_address_required, "destination address required",
	errc::device_or_resource_busy, "device or resource busy",
	errc::directory_not_empty, "directory not empty",
	errc::executable_format_error, "executable format error",
	errc::file_exists, "file exists",
	errc::file_too_large, "file too large",
	errc::filename_too_long, "filename too long",
	errc::function_not_supported, "function not supported",
	errc::host_unreachable, "host unreachable",
	errc::identifier_removed, "identifier removed",
	errc::illegal_byte_sequence, "illegal byte sequence",
	errc::inappropriate_io_control_operation,
		"inappropriate io control operation",
	errc::interrupted, "interrupted",
	errc::invalid_argument, "invalid argument",
	errc::invalid_seek, "invalid seek",
	errc::io_error, "io error",
	errc::is_a_directory, "is a directory",
	errc::message_size, "message size",
	errc::network_down, "network down",
	errc::network_reset, "network reset",
	errc::network_unreachable, "network unreachable",
	errc::no_buffer_space, "no buffer space",
	errc::no_child_process, "no child process",
	errc::no_link, "no link",
	errc::no_lock_available, "no lock available",
	errc::no_message_available, "no message available",
	errc::no_message, "no message",
	errc::no_protocol_option, "no protocol option",
	errc::no_space_on_device, "no space on device",
	errc::no_stream_resources, "no stream resources",
	errc::no_such_device_or_address, "no such device or address",
	errc::no_such_device, "no such device",
	errc::no_such_file_or_directory, "no such file or directory",
	errc::no_such_process, "no such process",
	errc::not_a_directory, "not a directory",
	errc::not_a_socket, "not a socket",
	errc::not_a_stream, "not a stream",
	errc::not_connected, "not connected",
	errc::not_enough_memory, "not enough memory",
	errc::not_supported, "not supported",
	errc::operation_canceled, "operation canceled",
	errc::operation_in_progress, "operation in progress",
	errc::operation_not_permitted, "operation not permitted",
	errc::operation_not_supported, "operation not supported",
	errc::operation_would_block, "operation would block",
	errc::owner_dead, "owner dead",
	errc::permission_denied, "permission denied",
	errc::protocol_error, "protocol error",
	errc::protocol_not_supported, "protocol not supported",
	errc::read_only_file_system, "read only file system",
	errc::resource_deadlock_would_occur, "resource deadlock would occur",
	errc::resource_unavailable_try_again, "resource unavailable try again",
	errc::result_out_of_range, "result out of range",
	errc::state_not_recoverable, "state not recoverable",
	errc::stream_timeout, "stream timeout",
	errc::text_file_busy, "text file busy",
	errc::timed_out, "timed out",
	errc::too_many_files_open_in_system, "too many files open in system",
	errc::too_many_files_open, "too many files open",
	errc::too_many_links, "too many links",
	errc::too_many_symbolic_link_levels, "too many symbolic link levels",
	errc::value_too_large, "value too large",
	errc::wrong_protocol_type, "wrong protocol type",
	(generic_errno)0, 0
	};

const char *__cdecl _Syserror_map(int _Errcode)
	{	// convert to name of generic error
	const _Sys_errtab_t *_Ptr = &_Sys_errtab[0];
	for (; _Ptr->_Name != 0; ++_Ptr)
		if ((int)_Ptr->_Errcode == _Errcode)
			return (_Ptr->_Name);
	return (0);
	}
}
#endif


#endif
