from string import Template
count = 12
EXPECTED_NN = 5
header = """
version: '3.7'
services:
"""

s = """
  node${INDEX}:
    image: spkdfs:latest
    command:
      [
        "-nodes=${IPS}",
        "-nn_port=8000",
        "-dn_port=8001",
        "-data_dir=/spkdfs/data",
        "-log_dir=/spkdfs/logs",
        "-coredumps_dir=/spkdfs/coredumps",
        "-expected_nn=${EXPECTED_NN}"
      ]
    volumes:
      - ./tmp/spkdfs_${INDEX}/data:/spkdfs/data
      - ./tmp/spkdfs_${INDEX}/coredumps:/spkdfs/coredumps
      - ./tmp/spkdfs_${INDEX}/logs:/spkdfs/logs
    ports:
      - "${INDEX}800:8000"
      - "${INDEX}801:8001"
    networks:
      spkdfs_net:
        ipv4_address: ${IP}
"""
tail = """
networks:
  spkdfs_net:
    driver: bridge
    ipam:
      config:
        - subnet: 192.168.88.0/24
          gateway: 192.168.88.1
"""

template = Template(s)
IPS = []
for i in range(1, count+1):
    IPS.append("192.168.88.1{:02}".format(i))
with open("docker-compose.yml", 'w') as f:
    f.write(header)
    for i in range(1, count+1):
        rendered_str = template.substitute(
            INDEX=i, IPS=",".join(IPS), IP=IPS[i-1], EXPECTED_NN=EXPECTED_NN)
        f.write(rendered_str)
    f.write(tail)
