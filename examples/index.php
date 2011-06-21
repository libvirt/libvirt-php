<?php
	require('libvirt.php');
	$lv = new Libvirt('qemu:///system');
	$hn = $lv->get_hostname();
	if ($hn == false)
		die('Cannot open connection to hypervisor</body></html>');

	$action = array_key_exists('action', $_GET) ? $_GET['action'] : false;
	$subaction = array_key_exists('subaction', $_GET) ? $_GET['subaction'] : false;

	if (($action == 'get-screenshot') && (array_key_exists('uuid', $_GET))) {
		if (array_key_exists('width', $_GET) && $_GET['width'])
			$tmp = $lv->domain_get_screenshot_thumbnail($_GET['uuid'], $_GET['width']);
		else
			$tmp = $lv->domain_get_screenshot($_GET['uuid']);

                if (!$tmp)
			echo $lv->get_last_error().'<br />';
		else {
			Header('Content-Type: image/png');
			die($tmp);
		}
	}
?>
<html>
	<head>
		<title>Virtual server management</title>
	</head>
<body>
<?php
	require('header.php');

	$spaces = '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';

	$uri = $lv->get_uri();
	$tmp = $lv->get_domain_count();

	if ($action == 'storage-pools') {
		$msg = '';
		if ($subaction == 'volume-delete') {
			if ((array_key_exists('confirm', $_GET)) && ($_GET['confirm'] == 'yes'))
				$msg = $lv->storagevolume_delete( base64_decode($_GET['path']) ) ? 'Volume has been deleted successfully' : 'Cannot delete volume';
			else
				$msg = '<table>
					<tr>
						<td colspan="2">
							<b>Do you really want to delete volume <i>'.base64_decode($_GET['path']).'</i> ?</b><br />
						</td>
					</tr>
					<tr align="center">
						<td>
							<a href="'.$_SERVER['REQUEST_URI'].'&amp;confirm=yes">Yes, delete it</a>
						</td>
						<td>
							<a href="?action='.$action.'">No, go back</a>
						</td>
					</tr>';
		}
		else if ($subaction == 'volume-create') {
			if (array_key_exists('sent', $_POST)) {
					$msg = $lv->storagevolume_create($_GET['pool'], $_POST['name'], $_POST['capacity'], $_POST['allocation']) ?
								'Volume has been created successfully' : 'Cannot create volume';
			}
			else
			$msg = '<h3>Create a new volume</h3><form method="POST">
					<table>
						<tr>
							<td>Volume name: </td>
							<td><input type="text" name="name"></td>
						</tr>
						<tr>
							<td>Capacity (e.g. 10M or 1G): </td>
							<td><input type="text" name="capacity"></td>
						</tr>
						<tr>
							<td>Allocation (e.g. 10M or 1G): </td>
							<td><input type="text" name="allocation"></td>
						</tr>
						<tr align="center">
							<td colspan="2"><input type="submit" value=" Add storage volume "></td>
						</tr>
						<input type="hidden" name="sent" value="1" />
					</table>
				</form>';
		}

		echo "<h2>Storage pools</h2>";
		echo "Here's the list of the storage pools available on the connection.<br /><br />";

		echo "<table>";
		echo "<tr>
			<th>Name<th>
			<th>Activity</th>
			<th>Volume count</th>
			<th>State</th>
			<th>Capacity</th>
			<th>Allocation</th>
			<th>Available</th>
			<th>Path</th>
			<th>Permissions</th>
			<th>Ownership</th>
			<th>Actions</th>
		      </tr>";

		$pools = $lv->get_storagepools();
		for ($i = 0; $i < sizeof($pools); $i++) {
			$info = $lv->get_storagepool_info($pools[$i]);
			$act = $info['active'] ? 'Active' : 'Inactive';

			echo "<tr align=\"center\">
				<td>$spaces{$pools[$i]}$spaces<td>
				<td>$spaces$act$spaces</td>
				<td>$spaces{$info['volume_count']}$spaces</td>
                	        <td>$spaces{$lv->translate_storagepool_state($info['state'])}$spaces</td>
                        	<td>$spaces{$lv->format_size($info['capacity'], 2)}$spaces</td>
	                        <td>$spaces{$lv->format_size($info['allocation'], 2)}$spaces</td>
        	                <td>$spaces{$lv->format_size($info['available'], 2)}$spaces</td>
				<td>$spaces{$info['path']}$spaces</td>
				<td>$spaces{$lv->translate_perms($info['permissions'])}$spaces</td>
				<td>$spaces{$info['id_user']} / {$info['id_group']}$spaces</td>
				<td>$spaces<a href=\"?action=storage-pools&amp;pool={$pools[$i]}&amp;subaction=volume-create\">Create volume</a>$spaces</td>
                	      </tr>";

			if ($info['volume_count'] > 0) {
				echo "<tr>
					<td colspan=\"10\" style='padding-left: 40px'><table>
					<tr>
					  <th>Name</th>
					  <th>Type</th>
					  <th>Capacity</th>
					  <th>Allocation</th>
					  <th>Path</th>
					  <th>Actions</th>
					</tr>";
				$tmp = $lv->storagepool_get_volume_information($pools[$i]);
				$tmp_keys = array_keys($tmp);
				for ($ii = 0; $ii < sizeof($tmp); $ii++) {
					$path = base64_encode($tmp[$tmp_keys[$ii]]['path']);
					echo "<tr>
						<td>$spaces{$tmp_keys[$ii]}$spaces</td>
						<td>$spaces{$lv->translate_volume_type($tmp[$tmp_keys[$ii]]['type'])}$spaces</td>
						<td>$spaces{$lv->format_size($tmp[$tmp_keys[$ii]]['capacity'], 2)}$spaces</td>
						<td>$spaces{$lv->format_size($tmp[$tmp_keys[$ii]]['allocation'], 2)}$spaces</td>
						<td>$spaces{$tmp[$tmp_keys[$ii]]['path']}$spaces</td>
						<td>$spaces<a href=\"?action=storage-pools&amp;path=$path&amp;subaction=volume-delete\">Delete volume</a>$spaces</td>
					      </tr>";
				}

				echo "</table></td>
					</tr>";
			}
		}
		echo "</table>";

		echo (strpos($msg, '<form')) ? $msg : '<pre>'.$msg.'</pre>';
	}
	else
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
		if ($subaction) {
			$name = $_GET['name'];
			if ($subaction == 'start')
				$ret = $lv->set_network_active($name, true) ? "Network has been started successfully" : 'Error while starting network: '.$lv->get_last_error();
			else
			if ($subaction == 'stop')
				$ret = $lv->set_network_active($name, false) ? "Network has been stopped successfully" : 'Error while stopping network: '.$lv->get_last_error();
			else
			if (($subaction == 'dumpxml') || ($subaction == 'edit')) {
                                $xml = $lv->network_get_xml($name, false);

                                if ($subaction == 'edit') {
                                        if (@$_POST['xmldesc']) {
                                                $ret = $lv->network_change_xml($name, $_POST['xmldesc']) ? "Network definition has been changed" :
                                                                                                        'Error changing network definition: '.$lv->get_last_error();
                                        }
                                        else
                                                $ret = 'Editing network XML description: <br /><br /><form method="POST"><table><tr><td>Network XML description: </td>'.
                                                        '<td><textarea name="xmldesc" rows="25" cols="90%">'.$xml.'</textarea></td></tr><tr align="center"><td colspan="2">'.
                                                        '<input type="submit" value=" Edit domain XML description "></tr></form>';
                                }
                                else
                                        $ret = 'XML dump of network <i>'.$name.'</i>:<br /><br />'.htmlentities($lv->get_network_xml($name, false));
                        }
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
			$act .= " | <a href=\"?action={$_GET['action']}&amp;subaction=dumpxml&amp;name={$tmp2['name']}\">Dump network XML</a>";
			if (!$tmp2['active']) {
				$act .= ' | <a href="?action='.$_GET['action'].'&amp;subaction=edit&amp;name='.$tmp2['name'].'">Edit network</a>';
			}

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
		$ci  = $lv->get_connect_information();

		$info = '';
		if ($ci['uri'])
			$info .= 'connected to <i>'.$ci['uri'].'</i> on <i>'.$ci['hostname'].'</i>, ';
		if ($ci['encrypted'] == 'Yes')
			$info .= 'encrypted, ';
		if ($ci['secure'] == 'Yes')
			$info .= 'secure, ';
		if ($ci['hypervisor_maxvcpus'])
			$info .= 'maximum '.$ci['hypervisor_maxvcpus'].' vcpus per guest, ';

		if (strlen($info) > 2)
			$info[ strlen($info) - 2 ] = ' ';

		echo "<h2>Host information</h2>
			  <table>
				<tr>
					<td>Hypervisor: </td>
					<td>{$ci['hypervisor_string']}</td>
				</tr>
				<tr>
					<td>Connection information: </td>
					<td>$info</td>
				</tr>
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
		$subaction = array_key_exists('subaction', $_GET) ? $_GET['subaction'] : false;
		$ret = false;
		$die = false;
		$domName = $lv->domain_get_name_by_uuid($_GET['uuid']);

		if ($subaction == 'disk-remove') {
			if ((array_key_exists('confirm', $_GET)) && ($_GET['confirm'] == 'yes'))
				$ret = $lv->domain_disk_remove($domName, $_GET['dev']) ? 'Disk has been removed successfully' : 'Cannot remove disk: '.$lv->get_last_error();
			else {
				$ret = '<table>
					<tr>
					<td colspan="2">
						<b>Do you really want to delete disk <i>'.$_GET['dev'].' from the guest</i> ?</b><br />
					</td>
					</tr>
					<tr align="center">
					<td>
						<a href="'.$_SERVER['REQUEST_URI'].'&amp;confirm=yes">Yes, delete it</a>
					</td>
					<td>
						 <a href="?action='.$action.'&amp;uuid='.$_GET['uuid'].'">No, go back</a>
					</td>
					</tr>';
				$die = true;
			}
		}
		if ($subaction == 'disk-add') {
			$img = array_key_exists('img', $_POST) ? $_POST['img'] : false;

			if ($img)
				$ret = $lv->domain_disk_add($domName, $_POST['img'], $_POST['dev']) ? 'Disk has been successfully added to the guest' :
						'Cannot add disk to the guest: '.$lv->get_last_error();
                        else
				$ret = '<b>Add a new disk device</b>
				<form method="POST">
				<table>
				<tr>
					<td>Disk image: </td>
					<td><input type="text" name="img" /></td>
				</tr>
				<tr>
					<td>Disk device in the guest: </td>
					<td><input type="text" name="dev" value="hdb" /></td>
				</tr>
				<tr align="center">
					<td colspan="2"><input type="submit" value=" Add new disk " /></td>
				</tr>
				</table>
				</form>';
		}

		if ($subaction == 'nic-remove') {
			if ((array_key_exists('confirm', $_GET)) && ($_GET['confirm'] == 'yes'))
				$ret = $lv->domain_nic_remove($domName, $_GET['mac']) ? 'Network card has been removed successfully' : 'Cannot remove network card: '.$lv->get_last_error();
			else {
				$ret = '<table>
					<tr>
					<td colspan="2">
						<b>Do you really want to delete NIC with MAC address <i>'.$_GET['mac'].' from the guest</i> ?</b><br />
					</td>
					</tr>
					<tr align="center">
					<td>
						<a href="'.$_SERVER['REQUEST_URI'].'&amp;confirm=yes">Yes, delete it</a>
					</td>
					<td>
						 <a href="?action='.$action.'&amp;uuid='.$_GET['uuid'].'">No, go back</a>
					</td>
					</tr>';
				$die = true;
			}
		}
		if ($subaction == 'nic-add') {
			$mac = array_key_exists('mac', $_POST) ? $_POST['mac'] : false;

			if ($mac)
				$ret = $lv->domain_nic_add($domName, $_POST['mac'], $_POST['network'], $_POST['model']) ? 'Network card has been successfully added to the guest' :
						'Cannot add NIC to the guest: '.$lv->get_last_error();
                        else {
				$ret = '<b>Add a new NIC device</b>
				<form method="POST">
				<table>
				<tr>
					<td>Network card MAC address: </td>
					<td><input type="text" name="mac" value="'.$lv->generate_random_mac_addr().'" /></td>
				</tr>
				<tr>
					<td>Network: </td>
					<td>
						<select name="network">
						';
						
				$nets = $lv->get_networks();
				for ($i = 0; $i < sizeof($nets); $i++)
						$ret .= '<option value="'.$nets[$i].'">'.$nets[$i].'</option>';
				
				$ret .= '
						</select>
					</td>
				</tr>
				<tr>
					<td>Card model: </td>
					<td>
						<select name="model">
						';
						
				$models = $lv->get_nic_models();
				for ($i = 0; $i < sizeof($models); $i++)
						$ret .= '<option value="'.$models[$i].'">'.$models[$i].'</option>';
				
				$ret .= '
						</select>
					</td>
				</tr>
				<tr align="center">
					<td colspan="2"><input type="submit" value=" Add new network card " /></td>
				</tr>
				</table>
				</form>';
				}
		}

		$res = $lv->get_domain_object($domName);
		$dom = $lv->domain_get_info($res);
		$mem = number_format($dom['memory'] / 1024, 2, '.', ' ').' MB';
		$cpu = $dom['nrVirtCpu'];
		$state = $lv->domain_state_translate($dom['state']);
		$id = $lv->domain_get_id($res);
		$arch = $lv->domain_get_arch($res);
		$vnc = $lv->domain_get_vnc_port($res);

		if (!$id)
			$id = 'N/A';
		if ($vnc <= 0)
			$vnc = 'N/A';

		echo "<h2>$domName - domain information</h2>";
		echo "<b>Domain type: </b>".$lv->get_domain_type($domName).'<br />';
		echo "<b>Domain emulator: </b>".$lv->get_domain_emulator($domName).'<br />';
		echo "<b>Domain memory: </b>$mem<br />";
		echo "<b>Number of vCPUs: </b>$cpu<br />";
		echo "<b>Domain state: </b>$state<br />";
		echo "<b>Domain architecture: </b>$arch<br />";
		echo "<b>Domain ID: </b>$id<br />";
		echo "<b>VNC Port: </b>$vnc<br />";
		echo '<br />';

		echo $ret;
		if ($die)
			die('</body></html');

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
				<th>$spaces Actions $spaces</th>
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
							   <td align=\"center\">$spaces
									<a href=\"?action=$action&amp;uuid={$_GET['uuid']}&amp;subaction=disk-remove&amp;dev={$tmp[$i]['device']}\">
										Remove disk device</a>
							$spaces</td>
        	                  </tr>";
                	}
			echo "</table>";
		}
		else
			echo "Domain doesn't have any disk devices";

			echo "<br />$spaces<a href=\"?action=$action&amp;uuid={$_GET['uuid']}&amp;subaction=disk-add\">Add new disk</a>";

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
				<th>$spaces Actions $spaces</th>
	                      </tr>";

				for ($i = 0; $i < sizeof($tmp); $i++) {
					if (in_array($tmp[$i]['network'], $anets))
						$netUp = 'Yes';
					else
						$netUp = 'No <a href="?action=virtual-networks&amp;subaction=start&amp;name='.$tmp[$i]['network'].'">[Start]</a>';

					echo "<tr>
                	           <td>$spaces{$tmp[$i]['mac']}$spaces</td>
                        	   <td align=\"center\">$spaces{$tmp[$i]['nic_type']}$spaces</td>
	                           <td align=\"center\">$spaces{$tmp[$i]['network']}$spaces</td>
        	                   <td align=\"center\">$spaces$netUp$spaces</td>
							   <td align=\"center\">$spaces
									<a href=\"?action=$action&amp;uuid={$_GET['uuid']}&amp;subaction=nic-remove&amp;mac={$tmp[$i]['mac']}\">
										Remove network card</a>
							$spaces</td>        	                   
                	          </tr>";
				}
				echo "</table>";
				
				echo "<br />$spaces<a href=\"?action=$action&amp;uuid={$_GET['uuid']}&amp;subaction=nic-add\">Add new network card</a>";
			}
			else
				echo 'Domain doesn\'t have any network devices';

			if ( $dom['state'] == 1 ) {
				echo "<h3>Screenshot</h3><img src=\"?action=get-screenshot&uuid={$_GET['uuid']}&width=640\">";
			}
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
			else if (($action == 'domain-get-xml') || ($action == 'domain-edit')) {
				$inactive = (!$lv->domain_is_running($domName)) ? true : false;
				$xml = $lv->domain_get_xml($domName, $inactive);

				if ($action == 'domain-edit') {
					if (@$_POST['xmldesc']) {
						$ret = $lv->domain_change_xml($domName, $_POST['xmldesc']) ? "Domain definition has been changed" :
													'Error changing domain definition: '.$lv->get_last_error();
					}
					else
						$ret = 'Editing domain XML description: <br /><br /><form method="POST"><table><tr><td>Domain XML description: </td>'.
							'<td><textarea name="xmldesc" rows="25" cols="90%">'.$xml.'</textarea></td></tr><tr align="center"><td colspan="2">'.
							'<input type="submit" value=" Edit domain XML description "></tr></form>';
				}
				else
					$ret = "Domain XML for domain <i>$domName</i>:<br /><br />".htmlentities($xml);
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
				<th>ID / VNC port</th>";

		if (($tmp['active'] > 0) && ($lv->supports('screenshot')))
			echo "
				<th>Domain screenshot</th>
				";

		echo "
				<th>Action</th>
			  </tr>";

		$active = $tmp['active'];
		for ($i = 0; $i < sizeof($doms); $i++) {
			$name = $doms[$i];
			$res = $lv->get_domain_by_name($name);
			$uuid = libvirt_domain_get_uuid_string($res);
			$dom = $lv->domain_get_info($name);
			$mem = number_format($dom['memory'] / 1024, 2, '.', ' ').' MB';
			$cpu = $dom['nrVirtCpu'];
			$state = $lv->domain_state_translate($dom['state']);
			$id = $lv->domain_get_id($res);
			$arch = $lv->domain_get_arch($res);
			$vnc = $lv->domain_get_vnc_port($res);
			$nics = $lv->get_network_cards($res);
			if (($diskcnt = $lv->get_disk_count($res)) > 0) {
				$disks = $diskcnt.' / '.$lv->get_disk_capacity($res);
				$diskdesc = 'Current physical size: '.$lv->get_disk_capacity($res, true);
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
				";

			if (($active > 0) && ($lv->supports('screenshot')))
				echo "
					<td align=\"center\"><img src=\"?action=get-screenshot&uuid=$uuid&width=120\" id=\"screenshot$i\"></td>
				";

			echo "
					<td align=\"center\">$spaces
				";

			if ($lv->domain_is_running($name))
				echo "<a href=\"?action=domain-stop&amp;uuid=$uuid\">Stop domain</a> | <a href=\"?action=domain-destroy&amp;uuid=$uuid\">Destroy domain</a> |";
			else
				echo "<a href=\"?action=domain-start&amp;uuid=$uuid\">Start domain</a> |";

			echo "
						<a href=\"?action=domain-get-xml&amp;uuid=$uuid\">Dump domain</a>
				";

			if (!$lv->domain_is_running($name))
				echo "| <a href=\"?action=domain-edit&amp;uuid=$uuid\">Edit domain XML</a>";
			else
			if ($active > 0)
				echo "| <a href=\"?action=get-screenshot&amp;uuid=$uuid\">Get screenshot</a>";

			echo "
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
