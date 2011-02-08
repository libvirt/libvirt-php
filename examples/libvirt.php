<?php
	class Libvirt {
		private $conn;
		private $last_error;

		function Libvirt($uri = false) {
			if ($uri != false)
				$this->connect($uri);
		}

		function connect($uri = 'null') {
			$this->conn=libvirt_connect($uri, false);
			if ($this->conn==false)
			{
				$this->last_error = libvirt_get_last_error();
				return false;
			}
		}

		function get_connection() {
			return $this->conn;
		}

		function get_hostname() {
			return libvirt_get_hostname($this->conn);
		}

		function get_domain_object($nameRes) {
			if (!is_resource($nameRes)) {
				$dom=libvirt_domain_lookup_by_name($this->conn, $nameRes);
				if (!$dom)
					return false;
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

			return libvirt_domain_xml_xpath($dom, $xpath, $flags); 
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
					echo libvirt_get_last_error().'<br />';
			}

			return $ret;
		}

                function get_nic_info($domain) {
                        $dom = $this->get_domain_object($domain);

                        $macs =  $this->get_xpath($dom, '//domain/devices/interface[@type="network"]/mac/@address', false);
			if (!$macs)
				return false;

			$ret = array();
			for ($i = 0; $i < $macs['num']; $i++) {
				$tmp = libvirt_domain_get_network_info($dom, $macs[$i]);
				if ($tmp)
					$ret[] = $tmp;
				else
					echo libvirt_get_last_error().'<br />';
			}

                        return $ret;
                }

                function get_domain_type($domain) {
                        $dom = $this->get_domain_object($domain);

                        $tmp = $this->get_xpath($dom, '//domain/@type', false);
                        if ($tmp['num'] == 0)
                            return false;

                        $ret = $tmp[0];
                        unset($tmp);

                        return $ret;
                }

                function get_domain_emulator($domain) {
                        $dom = $this->get_domain_object($domain);

                        $tmp =  $this->get_xpath($dom, '//domain/devices/emulator', false);
                        if ($tmp['num'] == 0)
                            return false;

                        $ret = $tmp[0];
                        unset($tmp);

                        return $ret;
                }

		function get_network_cards($domain) {
			$dom = $this->get_domain_object($domain);

			$nics =  $this->get_xpath($dom, '//domain/devices/interface[@type="network"]', false);
			if (!is_array($nics))
				return false;

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
			return libvirt_get_uri($this->conn);
		}

		function get_domain_count() {
			$ac = libvirt_get_active_domain_count($this->conn);
			$ic = libvirt_get_inactive_domain_count($this->conn);
			$tc = libvirt_get_domain_count($this->conn);

			return array(
					'active'   => $ac,
					'inactive' => $ic,
					'total'    => $tc
				    );
		}

		function get_domains() {
			$domNames = array();

			$doms = libvirt_list_domains($this->conn);
			foreach ($doms as $dom) {
				$tmp = libvirt_domain_get_uuid_string($dom);
				$domNames[$tmp] = libvirt_domain_get_name($dom);
			}

			ksort($domNames);
			return $domNames;
		}

		function get_networks($type = VIR_NETWORKS_ALL) {
			return libvirt_list_networks($this->conn, $type);
		}

		function get_network_res($network) {
			if ($network == false)
				return false;
			if (is_resource($network))
				return $network;

			return libvirt_network_get($this->conn, $network);
		}

		function get_network_bridge($network) {
			$res = $this->get_network_res($network);
			if ($res == false)
				return false;

			return libvirt_network_get_bridge($res);
		}

		function get_network_active($network) {
			$res = $this->get_network_res($network);
			if ($res == false)
				return false;

			return libvirt_network_get_active($res);
		}

		function set_network_active($network, $active = true) {
			$res = $this->get_network_res($network);
			if ($res == false)
				return false;

			if (!libvirt_network_set_active($res, $active ? 1 : 0)) {
				$this->last_error = libvirt_get_last_error($this->conn);
				return false;
			}

			return true;
		}

		function get_network_information($network) {
			$res = $this->get_network_res($network);
			if ($res == false)
				return false;

			$tmp = libvirt_network_get_information($res);
			$tmp['active'] = $this->get_network_active($res);
			return $tmp;
		}

		function get_network_xml($network) {
			$res = $this->get_network_res($network);
			if ($res == false)
				return false;

			return libvirt_network_get_xml($res);
		}

		function get_node_devices($dev = false) {
			return ($dev == false) ? libvirt_list_nodedevs($this->conn) : libvirt_list_nodedevs($this->conn, $dev);
		}

		function get_node_device_res($res) {
			if ($res == false)
				return false;
			if (is_resource($res))
				return $res;

			return libvirt_nodedev_get($this->conn, $res);
		}

		function get_node_device_caps($dev) {
			$dev = $this->get_node_device_res($dev);

			return libvirt_nodedev_capabilities($dev);
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

			return libvirt_nodedev_get_xml($dev);
		}

		function get_node_device_information($dev) {
			$dev = $this->get_node_device_res($dev);

			return libvirt_nodedev_get_information($dev);			
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

			return libvirt_domain_get_xml_desc($dom, $get_inactive ? VIR_DOMAIN_XML_INACTIVE : 0);
		}

		function domain_get_id($domain) {
			$dom = $this->get_domain_object($domain);
			if ((!$dom) || (!$this->domain_is_running($dom)))
				return false;

			return libvirt_domain_get_id($dom);
		}

		function domain_get_interface_stats($nameRes, $iface) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			return libvirt_domain_interface_stats($dom, $iface);
		}

		function domain_get_memory_stats($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			return libvirt_domain_memory_stats($dom);
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
			return libvirt_domain_define_xml($this->conn, $xml);
		}

		function domain_destroy($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			return libvirt_domain_destroy($dom);
		}

		function domain_reboot($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			return libvirt_domain_reboot($dom);
		}

		function domain_suspend($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			return libvirt_domain_suspend($dom);
		}

		function domain_resume($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			return libvirt_domain_resume($dom);
		}

		function domain_get_name_by_uuid($uuid) {
			$dom = libvirt_domain_lookup_by_uuid_string($this->conn, $uuid);
			if (!$dom)
				return false;
			return libvirt_domain_get_name($dom);
		}

		function domain_shutdown($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			return libvirt_domain_shutdown($dom);
		}

		function domain_undefine($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			return libvirt_domain_undefine($dom);
		}

		function domain_is_running($domain) {
			$dom = $this->get_domain_object($domain);
			if (!$dom)
				return false;

			$tmp = libvirt_domain_get_info($dom);
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
			return libvirt_node_get_info($this->conn);
		}
	}
?>
