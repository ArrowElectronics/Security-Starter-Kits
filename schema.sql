
/*revoke all on db_example.* from 'springuser'@'localhost';
grant select, insert, delete, update on db_example.* to 'springuser'@'localhost';
*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/
CREATE DATABASE IF NOT EXISTS `ssk-database`;
USE `ssk-database`;
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/
 CREATE TABLE `roles` (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(255) DEFAULT NULL,
  `role_type` varchar(64) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_roles_role_type` (`role_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/
 CREATE TABLE `users` (
  `userid` varchar(255) NOT NULL,
  `user_name` varchar(64) NOT NULL,
  `password` varchar(60) DEFAULT NULL,
  `reset_token` varchar(255) DEFAULT NULL,
  `token_expiry_date` datetime(6) DEFAULT NULL,
  `created_time` datetime(6) DEFAULT NULL,
  `modified_time` datetime(6) DEFAULT NULL,
  PRIMARY KEY (`userid`),
  UNIQUE KEY `uk_users_username` (`user_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/
CREATE TABLE `user_roles` (
  `user_id` varchar(255) NOT NULL,
  `role_id` int NOT NULL,
  PRIMARY KEY (`user_id`,`role_id`),
  KEY `FK_user_roles_roles` (`role_id`),
  CONSTRAINT `FK_user_roles_roles` FOREIGN KEY (`role_id`) REFERENCES `roles` (`id`),
  CONSTRAINT `FK_user_roles_users` FOREIGN KEY (`user_id`) REFERENCES `users` (`userid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/ 
 CREATE TABLE `userpassword_history` (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `user_id` varchar(255) NOT NULL,
  `password` varchar(60) NOT NULL,
  `created_time` datetime(6) DEFAULT NULL,
  `modified_time` datetime(6) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/
 CREATE TABLE `awsregion` (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `region_id` varchar(255) NOT NULL,
  `endpoint` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/ 
 CREATE TABLE `awsuser_token` (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `user_name` varchar(255) NOT NULL,
  `access_key_id` varchar(255) NOT NULL,
  `is_root_account` int NOT NULL DEFAULT '0',
  `secret_access_key` varchar(255) NOT NULL,
  `create_date` datetime(6) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/
/*
CREATE TABLE `address` (
  `id` bigint NOT NULL,
  `street` varchar(255) DEFAULT NULL,
  `city` varchar(255) DEFAULT NULL,
  `state` varchar(255) DEFAULT NULL,
  `country` varchar(255) DEFAULT NULL,
  `postal_code` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
*/