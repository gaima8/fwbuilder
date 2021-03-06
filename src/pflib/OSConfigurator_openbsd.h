/* 

                          Firewall Builder

                 Copyright (C) 2002,2009 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@vk.crocodile.org

  $Id$

  This program is free software which we release under the GNU General Public
  License. You may redistribute and/or modify this program under the terms
  of that license as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  To get a copy of the GNU General Public License, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef _OSCONFIGURATOR_OPENBSD_HH
#define _OSCONFIGURATOR_OPENBSD_HH


#include "OSConfigurator_bsd.h"
#include "OSData_pf.h"

namespace fwcompiler
{

    class OSConfigurator_openbsd : public OSConfigurator_bsd
    {

	public:

	virtual ~OSConfigurator_openbsd() {};
	OSConfigurator_openbsd(libfwbuilder::FWObjectDatabase *_db,
                               libfwbuilder::Firewall *fw,
                               bool ipv6_policy) : 
        OSConfigurator_bsd(_db, fw, ipv6_policy) {}

        virtual int prolog();

	virtual std::string myPlatformName();
	virtual std::string printKernelVarsCommands();
    };
};

#endif
