from string import Template
import argparse

# 创建解析器
parser = argparse.ArgumentParser(description='generate docker-compose.yml')

parser.add_argument('--count_d', type=int, default=12,
                    help='count of datanode')
parser.add_argument('--count_n', type=int, default=5,
                    help='count of namenode')

# 解析命令行参数
args = parser.parse_args()

COUNT_D = args.count_d
COUNT_N = args.count_n

assert (COUNT_D >= COUNT_N)
header = """
version: '3.7'
services:
"""

s = """
  node${INDEX}:
    image: spkdfs:latest
    hostname: node${INDEX}
    command:
      [
        "-nodes=${IPS}",
        "-nn_port=8000",
        "-dn_port=8001",
        "-data_dir=/spkdfs/data",
        "-log_dir=/spkdfs/logs",
        "-coredumps_dir=/spkdfs/coredumps",
        "-expected_nn=${COUNT_N}"
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
for i in range(1, COUNT_D+1):
    IPS.append("192.168.88.1{:02}".format(i))
with open("docker-compose.yml", 'w') as f:
    f.write(header)
    for i in range(1, COUNT_D+1):
        rendered_str = template.substitute(
            INDEX=i, IPS=",".join(IPS), IP=IPS[i-1], COUNT_N=COUNT_N)
        f.write(rendered_str)
    f.write(tail)
