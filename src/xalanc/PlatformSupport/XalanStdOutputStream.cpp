/*
 * The Apache Software License, Version 1.1
 *
 *
 * Copyright (c) 1999 The Apache Software Foundation.  All rights 
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:  
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xalan" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written 
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation and was
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */
// Class header file...
#include "XalanStdOutputStream.hpp"



#include <cerrno>



#if defined(XALAN_CLASSIC_IOSTREAMS)
#include <iostream.h>
#else
#include <iostream>
#endif



#include <xalanc/PlatformSupport/DOMStringHelper.hpp>



XALAN_CPP_NAMESPACE_BEGIN



XalanStdOutputStream::XalanStdOutputStream(StreamType&	theOutputStream) :
	XalanOutputStream(),
#if !defined(XALAN_NEWLINE_IS_CRLF)
	m_outputStream(theOutputStream)
#else
	m_outputStream(theOutputStream),
	m_newlineString(0),
	m_newlineStringLength(0)
#endif
{
	// This will make sure that cerr is not buffered...
	if (&m_outputStream == &XALAN_STD_QUALIFIER cerr)
	{
		setBufferSize(0);

#if defined(XALAN_NEWLINE_IS_CRLF)
		m_newlineString = s_nlString;
		m_newlineStringLength = s_nlStringLength;
#endif
	}
#if defined(XALAN_NEWLINE_IS_CRLF)
	else if (&m_outputStream == &XALAN_STD_QUALIFIER cout)
	{
		m_newlineString = s_nlString;
		m_newlineStringLength = s_nlStringLength;
	}
	else
	{
		m_newlineString = s_nlCRString;
		m_newlineStringLength = s_nlCRStringLength;
	}
#endif
}



XalanStdOutputStream::~XalanStdOutputStream()
{
}



#if defined(XALAN_NEWLINE_IS_CRLF)
void
XalanStdOutputStream::newline()
{
	assert(m_newlineString != 0 && length(m_newlineString) == m_newlineStringLength);

	write(m_newlineString, m_newlineStringLength);
}



const XalanDOMChar*
XalanStdOutputStream::getNewlineString() const
{
	assert(m_newlineString != 0);

	return m_newlineString;
}
#endif



void
XalanStdOutputStream::doFlush()
{
	// Don't try to flush if the stream is in a bad state...
	if(m_outputStream)
	{
		m_outputStream.flush();

		if(!m_outputStream)
		{
#if defined(XALAN_STRICT_ANSI_HEADERS)
			using namespace std;
#endif

			throw XalanStdOutputStreamWriteException(errno);
		}
	}
}



void
XalanStdOutputStream::writeData(
			const char*		theBuffer,
			size_type		theBufferLength)
{
	assert(StreamSizeType(theBufferLength) == theBufferLength);

	m_outputStream.write(theBuffer, StreamSizeType(theBufferLength));

	if(!m_outputStream)
	{
#if defined(XALAN_STRICT_ANSI_HEADERS)
		using namespace std;
#endif

		throw XalanStdOutputStreamWriteException(errno);
	}
}



static XalanDOMString
FormatMessageLocal(
			const char*		theMessage,
			int				theErrorCode)
{
	XalanDOMString	theResult(theMessage);

	append(theResult, ".  The error code was ");

	LongToDOMString(theErrorCode, theResult);

	append(theResult, ".");

	return theResult;
}



XalanStdOutputStream::XalanStdOutputStreamWriteException::XalanStdOutputStreamWriteException(
		int					theErrorCode) :
	XalanOutputStreamException(FormatMessageLocal("Error writing to standard stream!",
													   theErrorCode),
								    TranscodeFromLocalCodePage("XercesStdOutputStreamWriteException"))
{
}



XalanStdOutputStream::XalanStdOutputStreamWriteException::~XalanStdOutputStreamWriteException()
{
}



XALAN_CPP_NAMESPACE_END