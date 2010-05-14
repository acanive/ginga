/******************************************************************************
Este arquivo eh parte da implementacao do ambiente declarativo do middleware
Ginga (Ginga-NCL).

Direitos Autorais Reservados (c) 1989-2007 PUC-Rio/Laboratorio TeleMidia

Este programa eh software livre; voce pode redistribui-lo e/ou modificah-lo sob
os termos da Licenca Publica Geral GNU versao 2 conforme publicada pela Free
Software Foundation.

Este programa eh distribuido na expectativa de que seja util, porem, SEM
NENHUMA GARANTIA; nem mesmo a garantia implicita de COMERCIABILIDADE OU
ADEQUACAO A UMA FINALIDADE ESPECIFICA. Consulte a Licenca Publica Geral do
GNU versao 2 para mais detalhes.

Voce deve ter recebido uma copia da Licenca Publica Geral do GNU versao 2 junto
com este programa; se nao, escreva para a Free Software Foundation, Inc., no
endereco 59 Temple Street, Suite 330, Boston, MA 02111-1307 USA.

Para maiores informacoes:
ncl @ telemidia.puc-rio.br
http://www.ncl.org.br
http://www.ginga.org.br
http://www.telemidia.puc-rio.br
******************************************************************************
This file is part of the declarative environment of middleware Ginga (Ginga-NCL)

Copyright: 1989-2007 PUC-RIO/LABORATORIO TELEMIDIA, All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License version 2 as published by
the Free Software Foundation.
//jumping reserved_future_use (first 4 bits of data[pos])
This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License version 2 for more
details.

You should have received a copy of the GNU General Public License version 2
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

For further information contact:
ncl @ telemidia.puc-rio.br
http://www.ncl.org.br
http://www.ginga.org.br
http://www.telemidia.puc-rio.br
*******************************************************************************/

#include "../../../include/LDTLinkageDescriptor.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace tsparser {
namespace si {
namespace descriptors {
//TODO: test this class - no use of this descriptor on TS files
	LDTLinkageDescriptor::LDTLinkageDescriptor() {
		descriptorTag      = 0xDC;
		descriptorLength   = 0;
		descriptions       = NULL;
		originalNetworkdId = 0;
		originalServiceId  = 0;
		transportStreamId  = 0;
	}

	LDTLinkageDescriptor::~LDTLinkageDescriptor() {
		if(descriptions != NULL){
			for(int i = 0; i < descriptionsLength; ++i){
				if(*(descriptions[i])!= NULL){
					delete (*(descriptions[i]));
				}
				delete descriptions;
				descriptions == NULL;
			}
		}
	}
	unsigned char LDTLinkageDescriptor::getDescriptorTag(){
		return descriptorTag;
	}
	unsigned int LDTLinkageDescriptor::getDescriptorLength(){
		return (unsigned int)descriptorLength;
	}
	vector<Description*>* LDTLinkageDescriptor::getDescriptions() {
		return descriptions;
	}

	unsigned char LDTLinkageDescriptor::getDescriptorType(struct Description*
			description) {

		return (description->descriptorType);
	}
	unsigned char LDTLinkageDescriptor::getUserDefined(struct Description*
			description) {

		return (description->userDefined);
	}
	/*
	unsigned char* LDTLinkageDescriptor::getAllDescriptorType() {
    	unsigned char* allDescriptorType;

    	if(descriptionsLength == 0){
    		return NULL;
    	}
    	allDescriptorType = new unsigned char[descriptionsLength];
    	for(int i = 0; i < descriptionsLength; ++i){
    		allDescriptorType[i] = ((Description*)
    				(description[i]))->descriptorType;
    	}

    	return allDescriptorType;
	}
    unsigned char* LDTLinkageDescriptor::getAllUserDefined(){
    	unsigned char* allUserDefined;

    	if(descriptionsLength == 0){
    		return NULL;
    	}
    	allUserDefined = new unsigned char[descriptionsLength];
    	for(int i = 0; i < descriptionsLength; ++i){
    		allUserDefined[i] = ((Description*)(description[i]))->userDefined;
    	}

    	return allUserDefined;
    }
    */
	void LDTLinkageDescriptor::print(){
		cout << "LDTLinkageDescriptor::print..."<< endl;
	}
	size_t LDTLinkageDescriptor::process(char* data, size_t pos){
		struct Description* description;
		unsigned char descriptionsLength;

		descriptorLength = data[pos+1];
		pos += 2;

		originalServiceId = (((data[pos] << 8) & 0xFF00) ||
				(data[pos+1] & 0xFF));
		pos += 2;

		transportStreamId = (((data[pos] << 8) & 0xFF00) ||
				(data[pos+1] & 0xFF));
		pos += 2;

		originalNetworkdId = (((data[pos] << 8) & 0xFF00) ||
				(data[pos+1] & 0xFF));
		pos ++;

		descriptionsLength = (descriptorLength - 6) / 4;
		for(int i = 0; i < descriptionsLength; i++){
			if(descriptions == NULL){
				//descriptions = new struct Description[descriptionCount];
				descriptions = new vector<Description*>;
			}
			description = new struct description;

			pos++;
			description->descriptionId = (((data[pos] << 8) & 0xFF00) ||
					(data[pos+1] & 0xFF));
			pos += 2;
			description->descriptorType = (data[pos] & 0x0F);

			pos ++;
			description->userDefined = data[pos];

			//descriptions[i] =  description;
			descriptions.push_back(description);
		}
		return pos;
	}
}

}

}

}

}

}

}

}
