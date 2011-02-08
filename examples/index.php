<html>
<head><title>Virtual server management</title></head>
<body>
<?php
	require('header.php');
	require('libvirt.php');

	$spaces = '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';

	$lv = new Libvirt('qemu:///system');
	$action = array_key_exists('action', $_GET) ? $_GET['action'] : false;

	$hn = $lv->get_hostname();
	if ($hn == false)
		die('Cannot open connection to hypervisor</body></html>');

	$uri = $lv->get_uri();
	$tmp = $lv->get_domain_count();

	if ($action == 'node-devices') {
		echo "<h2>Node devices</h2>";
		echo "Here's the list of node devices available on the host machine. You can dump the information about the node devices there so you have simple way ".
			 "to check presence of the devices using this page.<br /><br />";

		$ret = false;
		if (array_key_exists('subaction', $_GET)) {
			$name = $_GET['name'];
			if ($_GET['subaction'] == 'dumpxml')
				$ret = 'XML dump of node device <i>'.$name.'</i>:<br /><br />'.htmlentities($lv->get_node_device_xml($name, false));
		}

		echo "Host node devices capability list: ";
		$tmp = $lv->get_node_device_cap_options();
		for ($i = 0; $i < sizeof($tmp); $i++) {
			echo "<a href=\"?action={$_GET['action']}&amp;cap={$tmp[$i]}\">{$tmp[$i]}</a>";
			if ($i < (sizeof($tmp) - 1))
				echo ', ';
		}
		echo "<br /><br />";

 		$tmp = $lv->get_node_devices( array_key_exists('cap', $_GET) ? $_GET['cap'] : false );
		echo "<table>
			<tr>
			 <th>Device name $spaces</th>
			 <th>$spaces Identification $spaces</th>
			 <th>$spaces Driver name $spaces</th>
			 <th>$spaces Vendor $spaces</th>
			 <th>$spaces Product $spaces</th>
			 <th>$spaces Action $spaces</th>
			</tr>";

		for ($i = 0; $i < sizeof($tmp); $i++) {
			$tmp2 = $lv->get_node_device_information($tmp[$i]);

			$act = !array_key_exists('cap', $_GET) ? "<a href=\"?action={$_GET['action']}&amp;subaction=dumpxml&amp;name={$tmp2['name']}\">Dump configuration</a>" :
				   "<a href=\"?action={$_GET['action']}&amp;subaction=dumpxml&amp;cap={$_GET['cap']}&amp;name={$tmp2['name']}\">Dump configuration</a>";

			if ($tmp2['capability'] == 'system') {
				$driver = '-';
				$vendor = array_key_exists('hardware_vendor', $tmp2) ? $tmp2['hardware_vendor'] : '';
				$serial = array_key_exists('hardware_version', $tmp2) ? $tmp2['hardware_version'] : '';
				$ident = $vendor.' '.$serial;
				$product = array_key_exists('hardware_serial', $tmp2) ? $tmp2['hardware_serial'] : 'Unknown';
			}
			else
			if ($tmp2['capability'] == 'net') {
				$ident = array_key_exists('interface_name', $tmp2) ? $tmp2['interface_name'] : '-';
				$driver = array_key_exists('capabilities', $tmp2) ? $tmp2['capabilities'] : '-';
				$vendor = 'Unknown';
				$product = 'Unknown';
			}
			else {
				$driver  = array_key_exists('driver_name', $tmp2) ? $tmp2['driver_name'] : 'None';
				$vendor  = array_key_exists('vendor_name', $tmp2) ? $tmp2['vendor_name'] : 'Unknown';
				$product = array_key_exists('product_name', $tmp2) ? $tmp2['product_name'] : 'Unknown';
				if (array_key_exists('vendor_id', $tmp2) && array_key_exists('product_id', $tmp2))
					$ident = $tmp2['vendor_id'].':'.$tmp2['product_id'];
				else
					$ident = '-';
			}

			echo "<tr>
					<td>$spaces{$tmp2['name']}$spaces</td>
					<td align=\"center\">$spaces$ident$spaces</td>
					<td align=\"center\">$spaces$driver$spaces</td>
					<td align=\"center\">$spaces$vendor$spaces</td>
					<td align=\"center\">$spaces$product$spaces</td>
					<td align=\"center\">$act</td>
			      </tr>";
		}
		echo "</table>";

		if ($ret)
			echo "<pre>$ret</pre>";
	}
	else
	if ($action == 'virtual-networks') {
		echo "<h2>Networks</h2>";
		echo "This is the administration of virtual networks. You can see all the virtual network being available with their settings. Please make sure you're using the right network for the purpose you need to since using the isolated network between two or multiple guests is providing the sharing option but internet connectivity will be disabled. Please enable internet services only on the guests that are really requiring internet access for operation like e.g. HTTP server or FTP server but you don't need to put the internet access to the guest with e.g. MySQL instance or anything that might be managed from the web-site. For the scenario described you could setup 2 network, internet and isolated, where isolated network should be setup on both machine with Apache and MySQL but internet access should be set up just on the machine with Apache webserver with scripts to remotely connect to MySQL instance and manage it (using e.g. phpMyAdmin). Isolated network is the one that's having forwarding column set to None.";

		$ret = false;
		if (array_key_exists('subaction', $_GET)) {
			$name = $_GET['name'];
			if ($_GET['subaction'] == 'start')
				$ret = $lv->set_network_active($name, true) ? "Network has been started successfully" : 'Error while starting network: '.$lv->get_last_error();
			else
			if ($_GET['subaction'] == 'stop')
				$ret = $lv->set_network_active($name, false) ? "Network has been stopped successfully" : 'Error while stopping network: '.$lv->get_last_error();
			else
			if ($_GET['subaction'] == 'dumpxml')
				$ret = 'XML dump of network <i>'.$name.'</i>:<br /><br />'.htmlentities($lv->get_network_xml($name, false));
		}

		echo "<h3>List of networks</h3>";
		$tmp = $lv->get_networks(VIR_NETWORKS_ALL);

		echo "<table>
			<tr>
			 <th>Network name $spaces</th>
			 <th>$spaces Network state $spaces</th>
			 <th>$spaces Gateway IP Address $spaces</th>
			 <th>$spaces IP Address Range $spaces</th>
			 <th>$spaces Forwarding $spaces</th>
			 <th>$spaces DHCP Range $spaces</th>
			 <th>$spaces Actions $spaces</th>
			</tr>";

		for ($i = 0; $i < sizeof($tmp); $i++) {
			$tmp2 = $lv->get_network_information($tmp[$i]);
			if ($tmp2['forwarding'] != 'None')
				$forward = $tmp2['forwarding'].' to '.$tmp2['forward_dev'];
			else
				$forward = 'None';
			if (array_key_exists('dhcp_start', $tmp2) && array_key_exists('dhcp_end', $tmp2))
				$dhcp = $tmp2['dhcp_start'].' - '.$tmp2['dhcp_end'];
			else
				$dhcp = 'Disabled';
			$activity = $tmp2['active'] ? 'Active' : 'Inactive';

			$act = !$tmp2['active'] ? "<a href=\"?action={$_GET['action']}&amp;subaction=start&amp;name={$tmp2['name']}\">Start network</a>" :
									  "<a href=\"?action={$_GET['action']}&amp;subaction=stop&amp;name={$tmp2['name']}\">Stop network</a>";
			$act .= " | <a href=\"?action={$_GET['action']}&amp;subaction=dumpxml&amp;name={$tmp2['name']}\">Dump network</a>";
			echo "<tr>
					<td>$spaces{$tmp2['name']}$spaces</td>
					<td align=\"center\">$spaces$activity$spaces</td>
					<td align=\"center\">$spaces{$tmp2['ip']}$spaces</td>
					<td align=\"center\">$spaces{$tmp2['ip_range']}$spaces</td>
					<td align=\"center\">$spaces$forward$spaces</td>
					<td align=\"center\">$spaces$dhcp$spaces</td>
					<td align=\"center\">$spaces$act$spaces</td>
			      </tr>";
		}
		echo "</table>";

		if ($ret)
			echo "<pre>$ret</pre>";
	}
	else
	if ($action == 'host-information') {
		$tmp = $lv->host_get_node_info();

		echo "<h2>Host information</h2>
			  <table>
				<tr>
					<td>Architecture: </td>
					<td>{$tmp['model']}</td>
				</tr>
				<tr>
					<td>Total memory installed: </td>
					<td>".number_format(($tmp['memory'] / 1048576), 2, '.', ' ')."GB </td>
				</tr>
				<tr>
					<td>Total processor count: </td>
					<td>{$tmp['cpus']}</td>
				</tr>
				<tr>
					<td>Processor speed: </td>
					<td>{$tmp['mhz']} MHz</td>
				</tr>
				<tr>
					<td>Processor nodes: </td>
					<td>{$tmp['nodes']}</td>
				</tr>
				<tr>
					<td>Processor sockets: </td>
					<td>{$tmp['sockets']}</td>
				</tr>
				<tr>
					<td>Processor cores: </td>
					<td>{$tmp['cores']}</td>
				</tr>
				<tr>
					<td>Processor threads: </td>
					<td>{$tmp['threads']}</td>
				</tr>
			  </table>
			";

		unset($tmp);
	}
        else
        if ($action == 'domain-information') {
		$domName = $lv->domain_get_name_by_uuid($_GET['uuid']);

		echo "<h2>$domName - domain disk information</h2>";
		echo "Domain type: ".$lv->get_domain_type($domName).'<br />';
		echo "Domain emulator: ".$lv->get_domain_emulator($domName).'<br />';
		echo '<br />';

		/* Disk information */
		echo "<h3>Disk devices</h3>";
		$tmp = $lv->get_disk_stats($domName);

		if (!empty($tmp)) {
			echo "<table>
        	              <tr>
                	        <th>Disk storage</th>
	                        <th>$spaces Storage driver type $spaces</th>
        	                <th>$spaces Domain device $spaces</th>
                	        <th>$spaces Disk capacity $spaces</th>
							<th>$spaces Disk allocation $spaces</th>
							<th>$spaces Physical disk size $spaces</th>
			      </tr>";

			for ($i = 0; $i < sizeof($tmp); $i++) {
				$capacity = $lv->format_size($tmp[$i]['capacity'], 2);
				$allocation = $lv->format_size($tmp[$i]['allocation'], 2);
				$physical = $lv->format_size($tmp[$i]['physical'], 2);
				$dev = (array_key_exists('file', $tmp[$i])) ? $tmp[$i]['file'] : $tmp[$i]['partition'];

				echo "<tr>
                        	   <td>$spaces".basename($dev)."$spaces</td>
	                           <td align=\"center\">$spaces{$tmp[$i]['type']}$spaces</td>
        	                   <td align=\"center\">$spaces{$tmp[$i]['device']}$spaces</td>
                	           <td align=\"center\">$spaces$capacity$spaces</td>
                        	   <td align=\"center\">$spaces$allocation$spaces</td>
	                           <td align=\"center\">$spaces$physical$spaces</td>
        	                  </tr>";
                	}
			echo "</table>";
		}
		else
			echo "Domain doesn't have any disk devices";

                /* Network interface information */
			echo "<h3>Network devices</h3>";
			$tmp = $lv->get_nic_info($domName);
			if (!empty($tmp)) {
				$anets = $lv->get_networks(VIR_NETWORKS_ACTIVE);
    	        echo "<table>
                	      <tr>
	                       <th>MAC Address</th>
        	               <th>$spaces NIC Type$spaces</th>
                	       <th>$spaces Network$spaces</th>
        	               <th>$spaces Network active$spaces</th>
	                      </tr>";

				for ($i = 0; $i < sizeof($tmp); $i++) {
					$netUp = (in_array($tmp[$i]['network'], $anets)) ? 'Yes' : 'No';
					echo "<tr>
                	           <td>$spaces{$tmp[$i]['mac']}$spaces</td>
                        	   <td align=\"center\">$spaces{$tmp[$i]['nic_type']}$spaces</td>
	                           <td align=\"center\">$spaces{$tmp[$i]['network']}$spaces</td>
        	                   <td align=\"center\">$spaces$netUp$spaces</td>
                	          </tr>";
				}
				echo "</table>";
			}
			else
				echo 'Domain doesn\'t have any network devices';
        }
	else {
		echo "Hypervisor URI: <i>$uri</i>, hostname: <i>$hn</i><br />
			  Statistics: {$tmp['total']} domains, {$tmp['active']} active, {$tmp['inactive']} inactive<br /><br />";

		$ret = false;
		if ($action) {
			$domName = $lv->domain_get_name_by_uuid($_GET['uuid']);

			if ($action == 'domain-start') {
				$ret = $lv->domain_start($domName) ? "Domain has been started successfully" : 'Error while starting domain: '.$lv->get_last_error();
			}
			else if ($action == 'domain-stop') {
				$ret = $lv->domain_shutdown($domName) ? "Domain has been stopped successfully" : 'Error while stopping domain: '.$lv->get_last_error();
			}
			else if ($action == 'domain-destroy') {
				$ret = $lv->domain_destroy($domName) ? "Domain has been destroyed successfully" : 'Error while destroying domain: '.$lv->get_last_error();
			}
			else if ($action == 'domain-get-xml') {
				$inactive = (!$lv->domain_is_running($domName)) ? true : false;
				$ret = "Domain XML for domain <i>$domName</i>:<br /><br />".htmlentities($lv->domain_get_xml($domName, $inactive));
			}
		}

		$doms = $lv->get_domains();
		$domkeys = array_keys($doms);
		echo "<table>
			   <tr>
				<th>Name</th>
				<th>CPU#</th>
				<th>Memory</th>
				<th>Disk(s)</th>
				<th>NICs</th>
				<th>Arch</th>
				<th>State</th>
				<th>ID / VNC port</th>
				<th>Action</th>
			  </tr>";
		for ($i = 0; $i < sizeof($doms); $i++) {
			$uuid = $domkeys[$i];
			$name = $doms[$uuid];
			$domr= $lv->get_domain_object($name);
			$dom = $lv->get_domain_info($name);
			$mem = ($dom[$name]['memory'] / 1024).' MB';
			$cpu = $dom[$name]['nrVirtCpu'];
			$state = $lv->domain_state_translate($dom[$name]['state']);
			$id = $lv->domain_get_id($domr);
			$arch = $lv->get_domain_arch($domr);
			$vnc = $lv->get_domain_vnc_port($domr);
			$nics = $lv->get_network_cards($domr);
			if (($diskcnt = $lv->get_disk_count($domr)) > 0) {
				$disks = $diskcnt.' / '.$lv->get_disk_capacity($domr);
				$diskdesc = 'Current physical size: '.$lv->get_disk_capacity($domr, true);
			}
			else {
				$disks = '-';
				$diskdesc = '';
			}

			if ($vnc < 0)
				$vnc = '-';
			else
				$vnc = $_SERVER['HTTP_HOST'].':'.$vnc;

			unset($tmp);
			if (!$id)
				$id = '-';
			unset($dom);

			echo "<tr>
					<td>$spaces
					<a href=\"?action=domain-information&amp;uuid=$uuid\">$name</a>
					$spaces</td>
					<td>$spaces$cpu$spaces</td>
					<td>$spaces$mem$spaces</td>
					<td align=\"center\" title='$diskdesc'>$spaces$disks$spaces</td>
					<td align=\"center\">$spaces$nics$spaces</td>
					<td>$spaces$arch$spaces</td>
					<td>$spaces$state$spaces</td>
					<td align=\"center\">$spaces$id / $vnc$spaces</td>
					<td align=\"center\">$spaces
				";

			if ($lv->domain_is_running($name))
				echo "<a href=\"?action=domain-stop&amp;uuid=$uuid\">Stop domain</a> | <a href=\"?action=domain-destroy&amp;uuid=$uuid\">Destroy domain</a> |";
			else
				echo "<a href=\"?action=domain-start&amp;uuid=$uuid\">Start domain</a> |";

			echo "
						<a href=\"?action=domain-get-xml&amp;uuid=$uuid\">Dump domain</a>
					$spaces
					</td>
				  </tr>";
		}
		echo "</table>";

		echo "<br /><pre>$ret</pre>";
	}
?>
</body>
</html>
