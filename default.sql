DROP TABLE IF EXISTS search;
CREATE TABLE search (hash TEXT, name TEXT, size INTEGER, server TEXT);
CREATE INDEX IF NOT EXISTS name_index ON search (name);
INSERT INTO search (hash, name, size, server)
VALUES ('FE0EADF36408C4BCCF423F2AC5D0A40C82712E37', 'Double Dragon.avi', '23842790', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('09BAB4B44A21EDD4D55E28F708B7AA5E055F97D5', 'Double Dragon 2.avi', '20576554', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('619FD447FCA4E7472A1865AD1B046BEB42E3BE94', 'Excitebike.avi', '21953008', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('957C6B75B422C3F6208B695418140DF2873BC5A9', 'Super Mario 2 (Jap).avi', '21208890', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('670FE1E57BCA4CC83E5ADB843BF53D6081F6C561', 'Kings of Power 4 Billion %.avi', '337080320', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('A37576E85A08FF43817C070D5E5D9991DE851062', 'Primer.avi', '734001152', 'topaz;emerald;ruby');
