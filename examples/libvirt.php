<?php
	class Libvirt {
		private $conn;
		private $last_error;

		function Libvirt($uri = false) {
			if ($uri != false)
				$this->connect($uri);
		}

		function _set_last_error()
		{
			$this->last_error = libvirt_get_last_error();
			return false;
		}

		function connect($uri = 'null') {
			$this->conn=libvirt_connect($uri, false);
			if ($this->conn==false)
				return $this->_set_last_error();
		}

                function domain_disk_add($domain, $img, $dev, $type='scsi') {
                        $dom = $this->get_domain_object($domain);

                        $tmp = libvirt_domain_disk_add($dom, $img, $dev, $type);
                        return ($tmp) ? $tmp : $this->_set_last_error();
                }

		function domain_disk_remove($domain, $dev) {
			$dom = $this->get_domain_object($domain);

			$tmp = libvirt_domain_disk_remove($dom, $dev);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_connection() {
			return $this->conn;
		}

		function get_hostname() {
			return libvirt_connect_get_hostname($this->conn);
		}

		function get_domain_object($nameRes) {
			if (!is_resource($nameRes)) {
				$dom=libvirt_domain_lookup_by_name($this->conn, $nameRes);
				if (!$dom)
					return $this->_set_last_error();
			}
			else
				$dom=$nameRes;

			return $dom;
		}

		function get_xpath($domain, $xpath, $inactive = false) {
			$dom = $this->get_domain_object($domain);
			$flags = 0;
			if ($inactive)
				$flags = VIR_DOMAIN_XML_INACTIVE;

			$tmp = libvirt_domain_xml_xpath($dom, $xpath, $flags); 
			if (!$tmp)
				return $this->_set_last_error();

			return $tmp;
		}

		function get_disk_stats($domain) {
			$dom = $this->get_domain_object($domain);

			$disks =  $this->get_xpath($dom, '//domain/devices/disk[@device="disk"]/target/@dev', false);
			// create image as: qemu-img create -f qcow2 -o backing_file=RAW_IMG OUT_QCOW_IMG SIZE[K,M,G suffixed]

			$ret = array();
			for ($i = 0; $i < $disks['num']; $i++) {
				$tmp = libvirt_domain_get_block_info($dom, $disks[$i]);
				if ($tmp)
					$ret[] = $tmp;
				else
					$this->_set_last_error();
			}

			return $ret;
		}

                function get_nic_info($domain) {
                        $dom = $this->get_domain_object($domain);

                        $macs =  $this->get_xpath($dom, '//domain/devices/interface[@type="network"]/mac/@address', false);
			if (!$macs)
				return $this->_set_last_error();

			$ret = array();
			for ($i = 0; $i < $macs['num']; $i++) {
				$tmp = libvirt_domain_get_network_info($dom, $macs[$i]);
				if ($tmp)
					$ret[] = $tmp;
				else
					$this->_set_last_error();
			}

                        return $ret;
                }

                function get_domain_type($domain) {
                        $dom = $this->get_domain_object($domain);

                        $tmp = $this->get_xpath($dom, '//domain/@type', false);
                        if ($tmp['num'] == 0)
                            return $this->_set_last_error();

                        $ret = $tmp[0];
                        unset($tmp);

                        return $ret;
                }

                function get_domain_emulator($domain) {
                        $dom = $this->get_domain_object($domain);

                        $tmp =  $this->get_xpath($dom, '//domain/devices/emulator', false);
                        if ($tmp['num'] == 0)
                            return $this->_set_last_error();

                        $ret = $tmp[0];
                        unset($tmp);

                        return $ret;
                }

		function get_network_cards($domain) {
			$dom = $this->get_domain_object($domain);

			$nics =  $this->get_xpath($dom, '//domain/devices/interface[@type="network"]', false);
			if (!is_array($nics))
				return $this->_set_last_error();

			return $nics['num'];
		}

		function get_disk_capacity($domain, $physical=false, $disk='*', $unit='?') {
			$dom = $this->get_domain_object($domain);
			$tmp = $this->get_disk_stats($dom);

			$ret = 0;
			for ($i = 0; $i < sizeof($tmp); $i++) {
				if (($disk == '*') || ($tmp[$i]['device'] == $disk))
					if ($physical)
						$ret += $tmp[$i]['physical'];
					else
						$ret += $tmp[$i]['capacity'];
			}
			unset($tmp);

			return $this->format_size($ret, 2, $unit);
		}

		function get_disk_count($domain) {
			$dom = $this->get_domain_object($domain);
			$tmp = $this->get_disk_stats($dom);
			$ret = sizeof($tmp);
			unset($tmp);

			return $ret;
		}

		function format_size($value, $decimals, $unit='?') {
			/* Autodetect unit that's appropriate */
			if ($unit == '?') {
				/* (1 << 40) is not working correctly on i386 systems */
				if ($value > 1099511627776)
					$unit = 'T';
				else
				if ($value > (1 << 30))
					$unit = 'G';
				else
				if ($value > (1 << 20))
					$unit = 'M';
				else
				if ($value > (1 << 10))
					$unit = 'K';
				else
					$unit = 'B';
			}

			$unit = strtoupper($unit);

			switch ($unit) {
				case 'T': return number_format($value / (float)1099511627776, $decimals, '.', ' ').' TB';
				case 'G': return number_format($value / (float)(1 << 30), $decimals, '.', ' ').' GB';
				case 'M': return number_format($value / (float)(1 << 20), $decimals, '.', ' ').' MB';
				case 'K': return number_format($value / (float)(1 << 10), $decimals, '.', ' ').' kB';
				case 'B': return $value.' B';
			}

			return false;
		}

		function get_uri() {
			$tmp = libvirt_connect_get_uri($this->conn);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_domain_count() {
			$tmp = libvirt_domain_get_counts($this->conn);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_storagepools() {
			$tmp = libvirt_list_storagepools($this->conn);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_storagepool_res($res) {
			if ($res == false)
				return false;
			if (is_resource($res))
				return $res;

			$tmp = libvirt_storagepool_lookup_by_name($this->conn, $res);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_storagepool_info($name) {
			if (!($res = $this->get_storagepool_res($name)))
				return false;

			$path = libvirt_storagepool_get_xml_desc($res, '/pool/target/path');
			if (!$path)
				return $this->_set_last_error();
			$perms = libvirt_storagepool_get_xml_desc($res, '/pool/target/permissions/mode');
			if (!$perms)
				return $this->_set_last_error();
			$otmp1 = libvirt_storagepool_get_xml_desc($res, '/pool/target/permissions/owner');
			if (!is_string($otmp1))
				return $this->_set_last_error();
			$otmp2 = libvirt_storagepool_get_xml_desc($res, '/pool/target/permissions/group');
			if (!is_string($otmp2))
				return $this->_set_last_error();
			$tmp = libvirt_storagepool_get_info($res);
			$tmp['volume_count'] = sizeof( libvirt_storagepool_list_volumes($res) );
			$tmp['active'] = libvirt_storagepool_is_active($res);
			$tmp['path'] = $path;
			$tmp['permissions'] = $perms;
			$tmp['id_user'] = $otmp1;
			$tmp['id_group'] = $otmp2;

			return $tmp;
		}

		function storagepool_get_volume_information($pool, $name=false) {
			if (!is_resource($pool))
				$pool = $this->get_storagepool_res($pool);
			if (!$pool)
				return false;

			$out = array();
			$tmp = libvirt_storagepool_list_volumes($pool);
			for ($i = 0; $i < sizeof($tmp); $i++) {
				if (($tmp[$i] == $name) || ($name == false)) {
					$r = libvirt_storagevolume_lookup_by_name($pool, $tmp[$i]);
					$out[$tmp[$i]] = libvirt_storagevolume_get_info($r);
					$out[$tmp[$i]]['path'] = libvirt_storagevolume_get_path($r);
					unset($r);
				}
			}

			return $out;
		}

		function storagevolume_delete($path) {
			$vol = libvirt_storagevolume_lookup_by_path($this->conn, $path);
			if (!libvirt_storagevolume_delete($vol))
				return $this->_set_last_error();

			return true;
		}

		function translate_volume_type($type) {
			if ($type == 1)
				return 'Block device';

			return 'File image';
		}

		function translate_perms($mode) {
			$mode = (string)((int)$mode);

			$tmp = '---------';

			for ($i = 0; $i < 3; $i++) {
				$bits = (int)$mode[$i];
				if ($bits & 4)
					$tmp[ ($i * 3) ] = 'r';
				if ($bits & 2)
					$tmp[ ($i * 3) + 1 ] = 'w';
				if ($bits & 1)
					$tmp[ ($i * 3) + 2 ] = 'x';
			}
			

			return $tmp;
		}

		function parse_size($size) {
			$unit = $size[ strlen($size) - 1 ];

			$size = (int)$size;
			switch (strtoupper($unit)) {
				case 'T': $size *= 1099511627776;
					  break;
				case 'G': $size *= 1073741824;
					  break;
				case 'M': $size *= 1048576;
					  break;
				case 'K': $size *= 1024;
					  break;
			}

			return $size;
		}

		function storagevolume_create($pool, $name, $capacity, $allocation) {
			$pool = $this->get_storagepool_res($pool);

			$capacity = $this->parse_size($capacity);
			$allocation = $this->parse_size($allocation);

			$xml = "<volume>\n".
                               "  <name>$name</name>\n".
                               "  <capacity>$capacity</capacity>\n".
                               "  <allocation>$allocation</allocation>\n".
                               "</volume>";

			$tmp = libvirt_storagevolume_create_xml($pool, $xml);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_connect_information() {
			$tmp = libvirt_connect_get_information($this->conn);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_change_xml($domain, $xml) {
			$dom = $this->get_domain_object($domain);

			if (!($old_xml = libvirt_domain_get_xml_desc($dom, NULL)))
				return $this->_set_last_error();
			if (!libvirt_domain_undefine($dom))
				return $this->_set_last_error();
			if (!libvirt_domain_define_xml($this->conn, $xml)) {
				$this->last_error = libvirt_get_last_error();
				libvirt_domain_define_xml($this->conn, $old_xml);
				return false;
			}

			return true;
		}

		function network_change_xml($network, $xml) {
			$net = $this->get_network_res($network);

			if (!($old_xml = libvirt_network_get_xml_desc($net, NULL))) {
				return $this->_set_last_error();
			}
			if (!libvirt_network_undefine($net)) {
				return $this->_set_last_error();
			}
			if (!libvirt_network_define_xml($this->conn, $xml)) {
				$this->last_error = libvirt_get_last_error();
				libvirt_network_define_xml($this->conn, $old_xml);
				return false;
			}

			return true;
		}

		function translate_storagepool_state($state) {
			switch ($state) {
				case 0: return 'Not running';
					break;
				case 1: return 'Building pool';
					break;
				case 2: return 'Running';
					break;
				case 3: return 'Running degraded';
					break;
				case 4: return 'Running but inaccessible';
					break;
			}

			return 'Unknown';
		}

		function get_domains() {
			$tmp = libvirt_list_domains($this->conn);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_domain_by_name($name) {
			$tmp = libvirt_domain_lookup_by_name($this->conn, $name);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_networks($type = VIR_NETWORKS_ALL) {
			$tmp = libvirt_list_networks($this->conn, $type);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_network_res($network) {
			if ($network == false)
				return false;
			if (is_resource($network))
				return $network;

			$tmp = libvirt_network_get($this->conn, $network);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_network_bridge($network) {
			$res = $this->get_network_res($network);
			if ($res == false)
				return false;

			$tmp = libvirt_network_get_bridge($res);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_network_active($network) {
			$res = $this->get_network_res($network);
			if ($res == false)
				return false;

			$tmp = libvirt_network_get_active($res);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function set_network_active($network, $active = true) {
			$res = $this->get_network_res($network);
			if ($res == false)
				return false;

			if (!libvirt_network_set_active($res, $active ? 1 : 0))
				return $this->_set_last_error();

			return true;
		}

		function get_network_information($network) {
			$res = $this->get_network_res($network);
			if ($res == false)
				return false;

			$tmp = libvirt_network_get_information($res);
			if (!$tmp)
				return $this->_set_last_error();
			$tmp['active'] = $this->get_network_active($res);
			return $tmp;
		}

		function get_network_xml($network) {
			$res = $this->get_network_res($network);
			if ($res == false)
				return false;

			$tmp = libvirt_network_get_xml_desc($res, NULL);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_node_devices($dev = false) {
			$tmp = ($dev == false) ? libvirt_list_nodedevs($this->conn) : libvirt_list_nodedevs($this->conn, $dev);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_node_device_res($res) {
			if ($res == false)
				return false;
			if (is_resource($res))
				return $res;

			$tmp = libvirt_nodedev_get($this->conn, $res);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_node_device_caps($dev) {
			$dev = $this->get_node_device_res($dev);

			$tmp = libvirt_nodedev_capabilities($dev);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_node_device_cap_options() {
			$all = $this->get_node_devices();

			$ret = array();
			for ($i = 0; $i < sizeof($all); $i++) {
				$tmp = $this->get_node_device_caps($all[$i]);

				for ($ii = 0; $ii < sizeof($tmp); $ii++)
					if (!in_array($tmp[$ii], $ret))
						$ret[] = $tmp[$ii];
			}

			return $ret;
		}

		function get_node_device_xml($dev) {
			$dev = $this->get_node_device_res($dev);

			$tmp = libvirt_nodedev_get_xml_desc($dev, NULL);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_node_device_information($dev) {
			$dev = $this->get_node_device_res($dev);

			$tmp = libvirt_nodedev_get_information($dev);			
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function get_domain_info($name = false) {
			$ret = array();

			if ($name != false) {
				$dom=libvirt_domain_lookup_by_name($this->conn, $name);
				if (!$dom)
					return false;

				$ret[$name] = libvirt_domain_get_info($dom);
				return $ret;
			}

			$doms = libvirt_list_domains($this->conn);
			foreach ($doms as $dom) {
				$tmp = libvirt_domain_get_name($dom);
				$ret[$tmp] = libvirt_domain_get_info($dom);
			}

			ksort($ret);
			return $ret;
		}

		function get_last_error() {
			return $this->last_error;
		}

		function domain_get_xml($domain, $get_inactive = false) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_get_xml_desc($dom, $get_inactive ? VIR_DOMAIN_XML_INACTIVE : 0);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function network_get_xml($network) {
			$net = $this->get_network_res($network);
			if (!$net)
				return false;

			$tmp = libvirt_network_get_xml_desc($net, NULL);
			return ($tmp) ? $tmp : $this->_set_last_error();;
		}

		function domain_get_id($domain) {
			$dom = $this->get_domain_object($domain);
			if ((!$dom) || (!$this->domain_is_running($dom)))
				return false;

			$tmp = libvirt_domain_get_id($dom);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_get_interface_stats($nameRes, $iface) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_interface_stats($dom, $iface);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_get_memory_stats($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_memory_stats($dom);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_start($nameOrXml) {
			$dom=libvirt_domain_lookup_by_name($this->conn, $nameOrXml);
			if ($dom) {
				$ret = libvirt_domain_create($dom);
				$this->last_error = libvirt_get_last_error();
				return $ret;
			}

			$ret = libvirt_domain_create_xml($this->conn, $nameOrXml);
			$this->last_error = libvirt_get_last_error();
			return $ret;
		}

		function domain_define($xml) {
			$tmp = libvirt_domain_define_xml($this->conn, $xml);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_destroy($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_destroy($dom);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_reboot($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_reboot($dom);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_suspend($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_suspend($dom);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_resume($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_resume($dom);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_get_name_by_uuid($uuid) {
			$dom = libvirt_domain_lookup_by_uuid_string($this->conn, $uuid);
			if (!$dom)
				return false;
			$tmp = libvirt_domain_get_name($dom);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_shutdown($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_shutdown($dom);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_undefine($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_undefine($dom);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}

		function domain_is_running($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_get_info($dom);
			if (!$tmp)
				return $this->_set_last_error();
			$ret = ( ($tmp['state'] == VIR_DOMAIN_RUNNING) || ($tmp['state'] == VIR_DOMAIN_BLOCKED) );
			unset($tmp);
			return $ret;
		}

		function domain_state_translate($state) {
			switch ($state) {
				case VIR_DOMAIN_RUNNING:  return 'running';
				case VIR_DOMAIN_NOSTATE:  return 'nostate';
				case VIR_DOMAIN_BLOCKED:  return 'blocked';
				case VIR_DOMAIN_PAUSED:   return 'paused';
				case VIR_DOMAIN_SHUTDOWN: return 'shutdown';
				case VIR_DOMAIN_SHUTOFF:  return 'shutoff';
				case VIR_DOMAIN_CRASHED:  return 'crashed';
			}

			return 'unknown';
		}

		function get_domain_vnc_port($domain) {
			$tmp = $this->get_xpath($domain, '//domain/devices/graphics/@port', false);
			$var = (int)$tmp[0];
			unset($tmp);

			return $var;
		}

		function get_domain_arch($domain) {
			$tmp = $this->get_xpath($domain, '//domain/os/type/@arch', false);
			$var = $tmp[0];
			unset($tmp);

			return $var;
		}

		function host_get_node_info() {
			$tmp = libvirt_node_get_info($this->conn);
			return ($tmp) ? $tmp : $this->_set_last_error();
		}
	}
?>
