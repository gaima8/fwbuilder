/* 

                          Firewall Builder

                 Copyright (C) 2002-2011 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@fwbuilder.org

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

#include <assert.h>

#include "OSConfigurator_freebsd.h"
#include "Configlet.h"
#include "interfaceProperties.h"
#include "interfacePropertiesObjectFactory.h"

#include "fwbuilder/Firewall.h"
#include "fwbuilder/FWOptions.h"
#include "fwbuilder/Interface.h"
#include "fwbuilder/IPv4.h"
#include "fwbuilder/FailoverClusterGroup.h"
#include "fwbuilder/StateSyncClusterGroup.h"

#include <algorithm>
#include <memory>


using namespace libfwbuilder;
using namespace fwcompiler;
using namespace std;

string OSConfigurator_freebsd::myPlatformName() { return "FreeBSD"; }

string OSConfigurator_freebsd::printKernelVarsCommands() 
{
    FWOptions* options = fw->getOptionsObject();
    std::auto_ptr<Configlet> kernel_vars;
    if (options->getBool("generate_rc_conf_file"))
    {
        kernel_vars = std::auto_ptr<Configlet>(
            new Configlet(fw, "freebsd", "rc_conf_kernel_vars"));
    } else
    {
        kernel_vars = std::auto_ptr<Configlet>(
            new Configlet(fw, "bsd", "kernel_vars"));
    }

    kernel_vars->removeComments();
    setKernelVariable(fw, "freebsd_ip_forward", kernel_vars.get());
    setKernelVariable(fw, "freebsd_ipv6_forward", kernel_vars.get());
    setKernelVariable(fw, "freebsd_ip_sourceroute", kernel_vars.get());
    //setKernelVariable(fw, "freebsd_ip_redirect", kernel_vars.get());
    return kernel_vars->expand().toStdString();
}

void OSConfigurator_freebsd::setKernelVariable(Firewall *fw,
                                               const string &var_name,
                                               Configlet *configlet)
{
    FWOptions* options = fw->getOptionsObject();

    if (options->getBool("generate_rc_conf_file"))
    {
        string s;
        s = options->getStr(var_name);
        if (!s.empty())
        {
            configlet->setVariable(QString("have_") + var_name.c_str(), 1);
            string yesno = (s=="1" || s=="on" || s=="On") ? "YES" : "NO";
            configlet->setVariable(QString(var_name.c_str()),
                                   QString(yesno.c_str()));
        }
    } else
        OSConfigurator_bsd::setKernelVariable(fw, var_name, configlet);
}

int OSConfigurator_freebsd::prolog()
{
    //printPathForAllTools("freebsd");
    //printFunctions();

    //processFirewallOptions();

    //configureInterfaces();

    return 0;
}

void OSConfigurator_freebsd::listAllInterfacesConfigLine(QStringList names,
                                                            bool ipv6)
{
    FWOptions* options = fw->getOptionsObject();
    if (options->getBool("generate_rc_conf_file"))
    {
        if (ipv6)
        {
            interface_configuration_lines <<
                QString("ipv6_network_interfaces=\"%1\"").arg(names.join(" "));
        } else
        {
            interface_configuration_lines <<
                QString("network_interfaces=\"%1\"").arg(names.join(" "));
        }
    }
}
        
void OSConfigurator_freebsd::updateAddressesOfInterface(
    Interface *iface, list<pair<InetAddr,InetAddr> > all_addresses)
{
    FWOptions* options = fw->getOptionsObject();
    if (options->getBool("generate_rc_conf_file"))
    {
        /*
         * lines in rc.conf have the following format:
         *
         * network_interfaces="ed0 ed1 lo0"
         * ifconfig_ed0="inet 192.0.2.1 netmask 0xffffff00"
         * ipv4_addrs_ed0="192.0.2.129/27 192.0.2.1-5/28"
         *
         */

        QString interface_name = iface->getName().c_str();

        if (iface->isDyn())
        {
            ifconfig_lines[interface_name] << "DHCP";
        }

        int ipv4_alias_counter = -2;
        int ipv6_alias_counter = -2;

        for (list<pair<InetAddr,InetAddr> >::iterator j = all_addresses.begin();
             j != all_addresses.end(); ++j)
        {
            QString ipv4_conf_line;
            QString ipv6_conf_line;

            InetAddr ipaddr = j->first;
            InetAddr ipnetm = j->second;

            if (ipaddr.isV6())
            {
                ipv6_conf_line += 
                    QString("%1/%2")
                    .arg(ipaddr.toString().c_str())
                    .arg(ipnetm.getLength());
                ipv6_alias_counter++;
            } else
            {
                int nbits = ipnetm.getLength();
                uint32_t netm = 0;
                while (nbits)
                {
                    netm = netm >> 1;
                    netm |= 1<<31;
                    nbits--;
                }

                ipv4_conf_line += 
                    QString("%1 netmask 0x%2")
                    .arg(ipaddr.toString().c_str())
                    .arg(netm, -8, 16);

                ipv4_alias_counter++;
            }

            if (!ipv4_conf_line.isEmpty())
            {
                QString suffix;
                if (ipv4_alias_counter>=0)
                    suffix = QString("_alias%1").arg(ipv4_alias_counter);
                ifconfig_lines[interface_name + suffix] << ipv4_conf_line;
            }

            if (!ipv6_conf_line.isEmpty())
            {
                QString suffix;
                if (ipv6_alias_counter>=0)
                    suffix = QString("_alias%1").arg(ipv6_alias_counter);
                ipv6_ifconfig_lines[interface_name + suffix] << ipv6_conf_line;
            }
        }
    } else
        OSConfigurator_bsd::updateAddressesOfInterface(iface, all_addresses);
}

void OSConfigurator_freebsd::listAllVlansConfgLine(QStringList vlan_names)
{
    FWOptions* options = fw->getOptionsObject();
    if (options->getBool("generate_rc_conf_file"))
    {
        ;
    } else
        interface_configuration_lines <<
            QString("sync_vlan_interfaces %1").arg(vlan_names.join(" "));
}

/*

 For rc.conf format:

 If a vlans_<interface> variable is set, a vlan(4) interface
 will be created for each item in the list with the vlandev
 argument set to interface.  If a vlan interface's name is a
 number, then that number is used as the vlan tag and the new
 vlan interface is named interface.tag.  Otherwise, the vlan
 tag must be specified via a vlan parameter in the
 create_args_<interface> variable.

 To create a vlan device named em0.101 on em0 with the vlan
 tag 101:

 vlans_em0="101"

 To create a vlan device named myvlan on em0 with the vlan tag
 102:

 vlans_em0="myvlan"
 create_args_myvlan="vlan 102"

 */
void OSConfigurator_freebsd::updateVlansOfInterface(Interface *iface,
                                                    QStringList vlan_names)
{
    FWOptions* options = fw->getOptionsObject();
    if (options->getBool("generate_rc_conf_file"))
    {
        QStringList outp;
        outp << QString("vlans_%1=\"%2\"").arg(iface->getName().c_str())
            .arg(vlan_names.join(" "));
        foreach(QString vlan_intf_name, vlan_names)
        {
            std::auto_ptr<interfaceProperties> int_prop(
                interfacePropertiesObjectFactory::getInterfacePropertiesObject(
                    fw->getStr("host_OS")));
            QString parent_name_from_regex;
            int vlan_id;
            if (int_prop->parseVlan(vlan_intf_name,
                                    &parent_name_from_regex, &vlan_id))
            {
                outp << QString("create_args_%1=\"vlan %2\"")
                    .arg(vlan_intf_name).arg(vlan_id);
            }
        }
        interface_configuration_lines <<  outp.join("\n");
    } else
        interface_configuration_lines <<
            QString("update_vlans_of_interface \"%1 %2\"")
            .arg(iface->getName().c_str())
            .arg(vlan_names.join(" "));
}

void OSConfigurator_freebsd::listAllBridgeConfgLine(QStringList bridge_names)
{
    FWOptions* options = fw->getOptionsObject();
    if (options->getBool("generate_rc_conf_file"))
    {
        cloned_interfaces += bridge_names;
    } else
        OSConfigurator_bsd::listAllBridgeConfgLine(bridge_names);
}

/*

 For rc.conf format:

     Consider a system with two 4-port Ethernet boards.  The following will
     cause a bridge consisting of all 8 ports with Rapid Spanning Tree enabled
     to be created:

           ifconfig bridge0 create
           ifconfig bridge0 \
               addm fxp0 stp fxp0 \
               addm fxp1 stp fxp1 \
               addm fxp2 stp fxp2 \
               addm fxp3 stp fxp3 \
               addm fxp4 stp fxp4 \
               addm fxp5 stp fxp5 \
               addm fxp6 stp fxp6 \
               addm fxp7 stp fxp7 \
               up

     The bridge can be used as a regular host interface at the same time as
     bridging between its member ports.  In this example, the bridge connects
     em0 and em1, and will receive its IP address through DHCP:

           cloned_interfaces="bridge0"
           ifconfig_bridge0="addm em0 addm em1 DHCP"
           ifconfig_em0="up"
           ifconfig_em1="up"


Refernce:
http://www.freebsd.org/doc/en_US.ISO8859-1/books/handbook/network-bridging.html

TODO:  STP support should be optional

 */
void OSConfigurator_freebsd::updateBridgeOfInterface(Interface *iface,
                                                     QStringList bridge_port_names)
{
    FWOptions* options = fw->getOptionsObject();
    if (options->getBool("generate_rc_conf_file"))
    {
        QStringList outp;
        QStringList bp;
        foreach(QString bridge_port, bridge_port_names)
        {
            bp << QString("addm %1 stp %2").arg(bridge_port).arg(bridge_port);
        }

        bp << "up";

        ifconfig_lines[iface->getName().c_str()] << bp.join(" ");

        foreach(QString bridge_port, bridge_port_names)
        {
            ifconfig_lines[bridge_port] << "up";
        }

        interface_configuration_lines <<  outp.join("\n");
    } else
        OSConfigurator_bsd::updateBridgeOfInterface(iface, bridge_port_names);
}


void OSConfigurator_freebsd::listAllCARPConfgLine(QStringList carp_names)
{
    FWOptions* options = fw->getOptionsObject();
    if (options->getBool("generate_rc_conf_file"))
    {
        cloned_interfaces += carp_names;
    } else
        OSConfigurator_bsd::listAllCARPConfgLine(carp_names);
}

void OSConfigurator_freebsd::updateCARPInterface(Interface *iface,
                                                 FWObject *failover_group)
{
    FWOptions* options = fw->getOptionsObject();
    if (options->getBool("generate_rc_conf_file"))
    {
        Configlet configlet(fw, "freebsd", "rc_conf_carp_interface");
        updateCARPInterfaceInternal(iface, failover_group, &configlet);
    } else
        OSConfigurator_bsd::updateCARPInterface(iface, failover_group);
}

void OSConfigurator_freebsd::listAllPfsyncConfgLine(bool have_pfsync)
{
    FWOptions* options = fw->getOptionsObject();
    if (options->getBool("generate_rc_conf_file"))
    {
        interface_configuration_lines <<  "pfsync_enable=\"YES\"";
    } else
        OSConfigurator_bsd::listAllPfsyncConfgLine(have_pfsync);
}

/*
  in rc.conf format:

     pfsync_enable
		 (bool) Set to ``NO'' by default.  Setting this to ``YES''
		 enables exposing pf(4) state changes to other hosts over the
		 network by means of pfsync(4).  The pfsync_syncdev variable
		 must also be set then.

     pfsync_syncdev
		 (str) Empty by default.  This variable specifies the name of
		 the network interface pfsync(4) should operate through.  It
		 must be set accordingly if pfsync_enable is set to ``YES''.

     pfsync_syncpeer
		 (str) Empty by default.  This variable is optional.  By
		 default, state change messages are sent out on the synchroni-
		 sation interface using IP multicast packets.  The protocol is
		 IP protocol 240, PFSYNC, and the multicast group used is
		 224.0.0.240.  When a peer address is specified using the
		 pfsync_syncpeer option, the peer address is used as a desti-
		 nation for the pfsync traffic, and the traffic can then be
		 protected using ipsec(4).  See the pfsync(4) manpage for more
		 details about using ipsec(4) with pfsync(4) interfaces.

     pfsync_ifconfig
		 (str) Empty by default.  This variable can contain additional
		 options to be passed to the ifconfig(8) command used to set
		 up pfsync(4).
 */

void OSConfigurator_freebsd::updatePfsyncInterface(
    Interface *iface, StateSyncClusterGroup *state_sync_group)
{
    FWOptions* options = fw->getOptionsObject();
    if (options->getBool("generate_rc_conf_file"))
    {
        Configlet configlet(fw, "freebsd", "rc_conf_pfsync_interface");
        configlet.removeComments();
        configlet.collapseEmptyStrings(true);
        configlet.setVariable("syncdev", iface->getName().c_str());

        if (state_sync_group->getOptionsObject()->getBool("syncpeer"))
        {
            for (FWObjectTypedChildIterator it =
                     state_sync_group->findByType(FWObjectReference::TYPENAME);
                 it != it.end(); ++it)
            {
                Interface *cluster_iface = Interface::cast(
                    FWObjectReference::getObject(*it));
                assert(cluster_iface);

                if (cluster_iface->getId() == iface->getId()) continue;
                    
                IPv4 *ipv4 = IPv4::cast(cluster_iface->getFirstByType(IPv4::TYPENAME));
                const InetAddr *addr = ipv4->getAddressPtr();

                configlet.setVariable("have_syncpeer", 1);
                configlet.setVariable("syncpeer", addr->toString().c_str());
            }
        }
        interface_configuration_lines <<  configlet.expand();

    } else
        OSConfigurator_bsd::updatePfsyncInterface(iface, state_sync_group);
}

QString OSConfigurator_freebsd::printAllInterfaceConfigurationLines()
{
    printIfconfigLines(ifconfig_lines);
    printIfconfigLines(ipv6_ifconfig_lines);

    if (!cloned_interfaces.isEmpty())
        interface_configuration_lines.push_front(
            QString("cloned_interfaces=\"%1\"").arg(cloned_interfaces.join(" ")));

    return interface_configuration_lines.join("\n");
}

void OSConfigurator_freebsd::printIfconfigLines(const QMap<QString, QStringList> &lines)
{
    if (!lines.isEmpty())
    {
        QMap<QString, QStringList>::const_iterator it;
        for (it=lines.begin(); it!=lines.end(); ++it)
        {
            const QString iface_name = it.key();
            const QStringList commands = it.value();
            interface_configuration_lines.push_front(
                QString("ifconfig_%1=\"%2\"").arg(iface_name)
                .arg(commands.join(" ")));
        }
    }
}

