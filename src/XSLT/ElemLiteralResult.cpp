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
#include "ElemLiteralResult.hpp"



#include <algorithm>



#include <sax/AttributeList.hpp>
#include <sax/SAXException.hpp>



#include <PlatformSupport/StringTokenizer.hpp>
#include <PlatformSupport/STLHelper.hpp>



#include <DOMSupport/DOMServices.hpp>



#include "AVT.hpp"
#include "Constants.hpp"
#include "Stylesheet.hpp"
#include "StylesheetConstructionContext.hpp"
#include "StylesheetExecutionContext.hpp"

#if !defined(XALAN_NO_NAMESPACES)
	using std::make_pair;
#endif


ElemLiteralResult::ElemLiteralResult(
			StylesheetConstructionContext&	constructionContext,
			Stylesheet&						stylesheetTree,
			const XalanDOMString&			name,
			const AttributeList&			atts,
			int								lineNumber,
			int								columnNumber,
			int								xslToken) :
	ElemUse(constructionContext,
			stylesheetTree,
			name,
			lineNumber,
			columnNumber,
			xslToken),
	m_QName(name)
{
	const unsigned int	nAttrs = atts.getLength();

	for(unsigned int i = 0; i < nAttrs; i++)
	{
		const XalanDOMChar*	const	aname = atts.getName(i);

		bool						needToProcess = true;
		const unsigned int			indexOfNSSep = indexOf(aname, ':');

		XalanDOMString				prefix;

		if(indexOfNSSep < length(aname))
		{
			prefix = substring(aname, 0, indexOfNSSep);

			if(!equals(prefix, DOMServices::s_XMLNamespace))
			{
				XalanDOMString	ns = getNamespaceForPrefix(prefix);

				if(equals(ns, stylesheetTree.getXSLTNamespaceURI()))
				{
					const XalanDOMString localName = substring(aname,indexOfNSSep + 1);

					processPrefixControl(localName, atts.getValue(i));

					if(0 != m_excludeResultPrefixes.size())
					{
						needToProcess = false;
					}
					else if (equals(localName, Constants::ATTRNAME_VERSION) == true)
					{
						const XalanDOMChar*	const	value = atts.getValue(i);

						stylesheetTree.setXSLTVerDeclared(DOMStringToDouble(value));
					}
				}
			}
			else
				// don't process namespace decls
				needToProcess = false;
		}

		if(needToProcess == true)
		{
			processSpaceAttr(aname, atts, i);

			// Add xmlns attribute(except xmlns:xsl), xml:space, etc... 
			// Ignore anything with xsl:xxx 
			if(! processUseAttributeSets(constructionContext, aname, atts, i) &&
					isAttrOK(aname, atts, i, constructionContext))
			{
				m_avts.push_back(new AVT(aname, atts.getType(i), atts.getValue(i), 	
							*this, constructionContext));
			}
		}
		removeExcludedPrefixes(m_excludeResultPrefixes);
	}
}



ElemLiteralResult::~ElemLiteralResult()
{
#if !defined(XALAN_NO_NAMESPACES)
	using std::for_each;
#endif

	// Clean up all entries in the vector.
	for_each(m_avts.begin(),
			 m_avts.end(),
			 DeleteFunctor<AVT>());
}



void ElemLiteralResult::execute(
			StylesheetExecutionContext&		executionContext,
			XalanNode*						sourceTree,
			XalanNode*						sourceNode,
			const QName&					mode) const
{
	executionContext.startElement(toCharArray(m_QName));
	
	ElemUse::execute(executionContext, sourceTree, sourceNode, mode);

	if(0 != m_avts.size())
	{
		const AVTVectorType::size_type	nAttrs = m_avts.size();

		for(AVTVectorType::size_type i = 0; i < nAttrs; i++)
		{
			const AVT* const	avt = m_avts[i];

			XalanDOMString	stringedValue;

			avt->evaluate(stringedValue, sourceNode, *this, executionContext);

			if(!isEmpty(stringedValue))
			{
				executionContext.replacePendingAttribute(toCharArray(avt->getName()), 
					avt->getType(), toCharArray(stringedValue));
			}
		}
	}

	// @@@ JMD:
	// This logic has been eliminated in the java version and replaced by a
	// method 'processResultNS' in the base class ElemTemplateElement
	// This also requires implementation of namespace alias logic
	const ElemTemplateElement*	elem = this;

	const NamespaceVectorType*	nsVector = &elem->getNameSpace();

	bool more = true;

	while(more == true)
	{
		for (NamespaceVectorType::size_type i = 0; i < nsVector->size(); i++)
		{
			NameSpace	ns = (*nsVector)[i];

			if(!isEmpty(ns.getURI()) && ns.getResultCandidate())
			{
				const bool		hasPrefix = !isEmpty(ns.getPrefix());

				XalanDOMString	prefix = hasPrefix ? ns.getPrefix() : XalanDOMString();
				XalanDOMString	desturi = executionContext.getResultNamespaceForPrefix(prefix);
				XalanDOMString	attrName = hasPrefix ? 
					DOMServices::s_XMLNamespaceWithSeparator + prefix :
							DOMServices::s_XMLNamespace;

				XalanDOMString srcURI = ns.getURI();

				bool isXSLNS = equals(srcURI, executionContext.getXSLNameSpaceURL())
					|| 0 != getStylesheet().lookupExtensionNSHandler(srcURI)
					|| equalsIgnoreCase(srcURI,executionContext.getXalanXSLNameSpaceURL());

				if(!isXSLNS && !equalsIgnoreCase(srcURI,desturi)) // TODO: Check for extension namespaces
				{
					executionContext.addResultAttribute(attrName, srcURI);
				}
			}
		}

		// We didn't find a namespace, start looking at the parents
		if (0 != elem)
		{
			elem = elem->getParentNodeElem();

			while(0 != elem)
			{
				nsVector = &elem->getNameSpace();

				if(0 == nsVector->size())
					elem = elem->getParentNodeElem();
				else
					break;
			}

			// Last chance, try the stylesheet namespace
			if (0 == nsVector || 0 == nsVector->size())
				nsVector = &getStylesheet().getNamespaceDecls();
			if (0 == nsVector || 0 == nsVector->size())
				more = false;
		}
		else
			more = false;
	}
/*
	java:
    // Handle namespaces(including those on the ancestor chain 
    // and stylesheet root declarations).
    processResultNS(processor);           
*/

	executeChildren(executionContext, sourceTree, sourceNode, mode);

	executionContext.endElement(toCharArray(m_QName));
}

/**
 * Process the exclude-result-prefixes or the extension-element-prefixes
 * attributes, for the purpose of prefix exclusion.
 */
void ElemLiteralResult::processPrefixControl(const DOMString& localName, 
		const DOMString& attrValue) 
{                                                                                                                   
/*
	OLD:
	if(equals(localName, Constants::ATTRNAME_EXTENSIONELEMENTPREFIXES))
	{
		needToProcess = false;
		const DOMString qnames = attrValue;
		StringTokenizer tokenizer(qnames, " \t\n\r", false);
		m_extensionElementPrefixes.reserve(tokenizer.countTokens());
		while(tokenizer.hasMoreTokens())
		{
			m_extensionElementPrefixes.push_back(tokenizer.nextToken());
		}
	}
*/
	if(equals(localName, Constants::ATTRNAME_EXTENSIONELEMENTPREFIXES) ||
			equals(localName, Constants::ATTRNAME_EXCLUDE_RESULT_PREFIXES))
	{
		const DOMString qnames = attrValue;
		StringTokenizer tokenizer(qnames, " \t\n\r", false);
		while(tokenizer.hasMoreTokens())
		{
			DOMString prefix = tokenizer.nextToken();
			if(equalsIgnoreCase(prefix, "#default")) prefix="";
			DOMString ns = getStylesheet().getNamespaceForPrefixFromStack(prefix);
			if(isEmpty(ns))
				throw SAXException("Invalid prefix in exclude-result-prefixes");
			m_excludeResultPrefixes.insert(make_pair(prefix, ns));
		}
	}
}

