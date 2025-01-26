CREATE TABLE `logs` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `type` int(4) NOT NULL DEFAULT 0,
  `interface` int(10) unsigned NOT NULL DEFAULT 0,
  `channel` char(8) NOT NULL DEFAULT '',
  `datetime` int(11) NOT NULL DEFAULT 0,
  `datetime_nsec` int(11) NOT NULL DEFAULT 0,
  `synced` int(11) NOT NULL DEFAULT 0,
  `data` varchar(256) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  KEY `synced` (`synced`)
) ENGINE=MyISAM AUTO_INCREMENT=0 DEFAULT CHARSET=utf8;

CREATE TABLE `interfaces` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `localid` varchar(40) NOT NULL DEFAULT '-1',
  `description` varchar(60) NOT NULL DEFAULT '',
  `serialno` char(30) NOT NULL DEFAULT '',
  `type` int(4) NOT NULL DEFAULT 0,
  `active` int(1) NOT NULL DEFAULT 0,
  `updated` int(11) NOT NULL DEFAULT 0,
  `updated_nsec` int(11) NOT NULL DEFAULT 0,
  `synced` int(11) NOT NULL DEFAULT 0,
  `contacted` int(11) NOT NULL DEFAULT 0,
  `errors` int(10) unsigned NOT NULL DEFAULT 0,
  `lasterror` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `localid` (`localid`),
  KEY `synced` (`synced`)
) ENGINE=MyISAM AUTO_INCREMENT=0 DEFAULT CHARSET=utf8;

CREATE TABLE `channels_digital` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `open_count` int(11) NOT NULL DEFAULT 0,
  `last_open` int(11) NOT NULL DEFAULT 0,
  `last_close` int(11) NOT NULL DEFAULT 0,
  `current_status` int(1) NOT NULL DEFAULT 0,
  `synced` int(11) NOT NULL DEFAULT 0,
  `contacted` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `synced` (`synced`)
) ENGINE=MyISAM AUTO_INCREMENT=0 DEFAULT CHARSET=utf8;

CREATE TABLE `channels_analog` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `current_status` double(10,7) NOT NULL DEFAULT 0.00000,
  `synced` int(11) NOT NULL DEFAULT 0,
  `contacted` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `synced` (`synced`)
) ENGINE=MyISAM AUTO_INCREMENT=0 DEFAULT CHARSET=utf8;

CREATE TABLE `channels` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `description` char(30) NOT NULL DEFAULT '',
  `interface` int(10) unsigned NOT NULL DEFAULT 0,
  `channel` char(8) NOT NULL DEFAULT '0',
  `type` int(4) NOT NULL DEFAULT 0,
  `direction` int(1) NOT NULL DEFAULT 0,
  `active` int(1) NOT NULL DEFAULT 0,
  `updated` int(11) NOT NULL DEFAULT 0,
  `synced` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `channel` (`channel`,`interface`),
  KEY `synced` (`synced`)
) ENGINE=MyISAM AUTO_INCREMENT=0 DEFAULT CHARSET=utf8;

CREATE TABLE `channel_types` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `interfacetype` int(4) unsigned NOT NULL DEFAULT 0,
  `channel` varchar(30) NOT NULL DEFAULT '',
  `fio` int(4) NOT NULL DEFAULT 0,
  `type` int(4) NOT NULL DEFAULT 0,
  `direction` int(1) NOT NULL DEFAULT 0,
  `updated` int(11) NOT NULL DEFAULT 0,
  `created` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `channel` (`channel`)
) ENGINE=MyISAM AUTO_INCREMENT=0 DEFAULT CHARSET=utf8 ;

