/* **********Roles***********/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/
INSERT IGNORE INTO roles(id,name,role_type) VALUES(1,'User','ROLE_USER');
INSERT IGNORE INTO roles(id,name,role_type) VALUES(2,'Tech Support','ROLE_TECH_SUPPORT');
INSERT IGNORE INTO roles(id,name,role_type) VALUES(3,'Admin','ROLE_ADMIN');
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/

INSERT IGNORE INTO users(userid,user_name,password,created_time,modified_time) values('AIDA6HTUWELNMZGRLETVN','seed.dev@einfochips.com','$2a$10$19V7qvr1NDsdQxAmtARtpuxHUPpBMRk01Jvhj.Mq0hDrzaN59vH3i',now(),now());
INSERT IGNORE INTO userpassword_history(user_id, password,created_time,modified_time) VALUES('AIDA6HTUWELNMZGRLETVN','$2a$10$19V7qvr1NDsdQxAmtARtpuxHUPpBMRk01Jvhj.Mq0hDrzaN59vH3i',now(),now());
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/

INSERT IGNORE INTO user_roles(user_id,role_id) values('AIDA6HTUWELNMZGRLETVN',1);

INSERT IGNORE INTO user_roles(user_id,role_id) values('AIDA6HTUWELNMZGRLETVN',2);

INSERT IGNORE INTO user_roles(user_id,role_id) values('AIDA6HTUWELNMZGRLETVN',3);
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/
INSERT IGNORE INTO awsuser_token(user_name,access_key_id,secret_access_key,is_root_account,create_date) values('seed.dev@einfochips.com','AKIA6HTUWELNLLZRXU7H','Uw5r/VdCMa7YggzkBP/PqwJrtlRHIe/X1F5WXHur',1,now());
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/
INSERT IGNORE INTO awsregion(region_id,endpoint) values('us-east-2','a3m3zunaszckgk-ats.iot.us-east-2.amazonaws.com');
/*-------------------------------------------------------------------------------------------------------------------------------------------------------+*/
INSERT IGNORE INTO bucketdetail(bucket_type,bucket_name) values('OTA_BUCKET','<ota bucket name>');
INSERT IGNORE INTO bucketdetail(bucket_type,bucket_name) values('LOG_BUCKET','<log bucket name>');