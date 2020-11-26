# workload

Disk I/O 부하(random 특성 부분) 테스트 유틸 workload

https://www.minzkn.com/moniwiki/wiki.php/workload
 

workload 의 목적

기존 공개된 대부분의 측정 툴은 Disk I/O 를 측정하지만 workload는 Filesystem 을 경유한 Random access 특성을 부여하여 기준을 측정하는 유일무이한 측정 툴입니다.
 
workload는 실행되는 위치에 있는 파일시스템의 디스크 I/O에 부하를 주는 프로그램입니다.

즉, /ferret 경로에 대한 디스크 부하를 주기 위해서는 /ferret으로 경로를 이동하여 실행합니다.

<pre>
$ cd /ferret
$ ./workload
=> 실행하면 제 장비 NGFW500에서는 다음과 같은 결과가 나옵니다. (읽기 1257.472KB/s, 쓰기 1343.488KB/s)
   [DURATION=10s] [RADIO=7:1] [        ] [       ] [ACCESS] [R:2854(1257.472KB/s)] [W:3079(1343.488KB/s)]
</pre>

이 명령을 통해서 디스크의 부하 성능이 어느정도인지를 확인할 수 있습니다.

여기서 디스크 부하방식은 Filesystem의 Random R/W access를 임의로 부하를 주는 방식입니다.

 

좀더 다양한 패턴의 설정으로 테스트가 가능하며 첨부파일 workload.xml을 내려받아 "/etc/workload.xml"로 저장하고 적절히 수정하여 사용할 수 있습니다.

기본적인 설정은 내장되어 있으며 "/etc/workload.xml"이 없는 경우 내부 기본 설정으로 동작하도록 되어 있습니다.

 

normal 실행
<pre>
$ cd /ferret
$ workload
</pre>

verbose mode로 실행 (기동시 자세한 초기화 과정이 표시됩니다.)
<pre>
$ cd /ferret
$ workload -v
</pre>

background 로 실행 ( 이렇게 실행하면 상태 view는 확인할 수 없지만 좀더 높은 성능으로 부하를 줄 수 있습니다. )
<pre>
$ cd /ferret
$ workload -d
</pre>

threads 수치 설정 (CPU Core 개수의 2배 이상을 지정하면 다소나마 부하를 높일 수 있습니다. threads를 1로 지정하)
<pre>
$ cd /ferret
$ workload --threads 16
</pre>

특정 시간동안만 부하를 주려면
<pre>
$ cd /ferret
$ workload --duration <msec>
</pre>

최대 부하를 주기위해서는 (threads는 Core의 2배수, filecount는 많을수록, filesize는 작을수록 단편화 심한 작업 => 랜덤 부하는 그 특성에 따른 parameter가 다를 필요가 있다는 것.)
<pre>
$ cd /ferret
$ workload -v --threads 64 --filecount 1000 --filesize 4096
</pre>
 

설정파일 예시 (붉은색 부분의 설정을 변경하여 테스트 방식을 조정가능)
<pre>
<?xml version="1.0" encoding="UTF-8"?>
<workload>
  <config>
    <!-- type=sqlite3|mysql -->
    <meta type="none">
      <!-- SQLite3 (too slow !!!)  -->
      <pathname>./workload.db</pathname>
      <!-- MySQL -->
      <hostname>127.0.0.1</hostname>
      <port>0</port>
      <username>workload</username>
      <password>workload</password>
      <database>workload</database>
    </meta>
    <operation pause="no">
      <left>7</left>
      <right>1</right>
      <interval>0</interval>

      <!-- maximum 1000000, assign=random|ordered|even -->
      <files remove="yes" assign="even">1000</files>

      <prefix>workload</prefix>
      <suffix>.bin</suffix>
      <filesize>4096</filesize>
      <duration>0</duration>

      <!-- (r)ead, (w)rite, rw -->
      <access sync="yes" sendfile="yes" threads="16">rw</access>
    </operation>
    <storage>
      <disk>
        <!-- <name>volume1</name> -->
        <path create="yes">./test/disk1</path>
      </disk>
      <disk>
        <path create="yes">./test/disk2</path>
      </disk>
      <disk>
        <path create="yes">./test/disk3</path>
      </disk>
      <disk>
        <path create="yes">./test/disk4</path>
      </disk>
      <disk>
        <path create="yes">./test/disk5</path>
      </disk>
      <disk>
        <path create="yes">./test/disk6</path>
      </disk>
      <disk>
        <path create="yes">./test/disk7</path>
      </disk>
      <disk>
        <path create="yes">./test/disk8</path>
      </disk>
      <disk>
        <path create="yes">./test/disk9</path>
      </disk>
      <disk>
        <path create="yes">./test/disk10</path>
      </disk>
      <disk>
        <path create="yes">./test/disk11</path>
      </disk>
      <disk>
        <path create="yes">./test/disk12</path>
      </disk>
      <disk>
        <path create="yes">./test/disk13</path>
      </disk>
      <disk>
        <path create="yes">./test/disk14</path>
      </disk>
      <disk>
        <path create="yes">./test/disk15</path>
      </disk>
      <disk>
        <path create="yes">./test/disk16</path>
      </disk>
    </storage>
  </config>
</workload>
<!-- vim: set expandtab: -->
<!-- End of config -->
</pre>
 

빌드방법
buildroot 에서 별도로 빌드하려면 "make -j64 package/do/workload/compile V=s" 로 빌드할 수 있습니다. 본래 독립 프로젝트이므로 buildroot 없이 workload 소스만 내려받아 빌드되는 것도 지원됩니다.

 

현재 workload 버젼 기준 (2019.03.20)
workload Ramdom access 방식 조정 옵션에 대한 주요 옵션 정리.
* --threads { I/O를 일으킬 thread 개수 }
* --filecount { 생성/삭제/변경등의 접근을 위한 임시 파일 개수 }
* --filesize { 하나의 파일에 접근시 단일 I/O 크기 }

---

측정 성능 수치가 높게 나오려면
1. thread 수치를 Core의 2배수로 설정 (단, 16이하인 경우 16으로 설정)
2. filecount 수치를 Core의 2배수로 설정
3. filesize 수치를 크게 잡을수록 (Ramdom 보다는 순차적인 접근률이 높아집니다.)

측정 수치가 극악으로 나오게 하려면
1. thread수치를 1
2. filecount 를 1 또는 매우 큰값
3. filesize를 1

순수 MySQL(None-thread, Table lock) 특성과 가장 유사한 접근을 위해서는 (추정사항입니다.)
1. thread 수치를 1
2. filecount를 10    
3. filesize를 4096

현재 workload 기본값 (즉, 아무런 인자 없이 실행하는 경우)
1. threads 16
2. filecount 1000
3. filesize 4096

추가적인 특성을 부여하기 위한 옵션

* -v : 초기 구동시 구동내용을 표시하고 syslog 로 관련 내용을 기록
* -d : view를 표시하지 않고 daemonize 로 실행 (순수 I/O를 일으키고 상태를 모니터링하지 않고자 할 때 사용)
* --left {value} --right {value} : 임시파일들을 균등하게 접근하지 않고 비율로 접근하는 값 (기본값은 7:1)
* --interval {msec} : I/O 접근 간격 강제 조정
* --duration {msec} : 측정 시간 (이 시간이 지나면 workload는 자동 종료)
* --assign-method {random|ordered|even} : 임시 파일을 disk경로 단위로 분산하는 알고리즘 선택 (기본값: even)
* --relocate : 임시 파일을 비율에 따라서 재배치(파일 move) 병행

그 밖에 본래 구현은 있으나 포함되지 않은 기능
* sqlite3 또는 Mysql 또는 MS-SQL 로 workload 실행 결과를 리포팅(push) 생성하는 기능 (이를 통해서 리포팅된 DB를 기반으로 분석하기 위한 기능)
** 중간중간에 리포팅을 모니터링하는 기능인 --dump-disk, --dump-file 옵션이 여기에 관여함.

기타 측정에 대한
첨부파일 netlink_iotop 은(첨부파일) 현재 실행되고 있는 각 프로세스별 R/W 총량을 정보로 표시
즉, process 중에서 어떤게 제일 많은 I/O를 일으켰는가를 실시간이 아닌 누적 총량으로 확인
iotop 명령은 각 프로세스별 R/W 속도와 총합에 대한 속도를 실시간 정보로 표시
Generic Netlink protocol을 이용한 TASKSTATS정보로 계산
"/proc/{pid}/io" 정보를 활용하는 방법도 있으나 iotop은 Netlink 로 구현됨.
dd 명령은 Sequential 한 Write 성능을 측정
</pre>
dd if=/dev/zero of={device 또는 파일} bs={block size} count={count} conv=fsync
</pre>
conv=fsync 옵션 필수. (이 옵션이 주어지지 않으면 cache로 인하여 I/O 측정 무의미)
hdparm/sdparm/smartctl 명령등은 Filesystem 과 상관없는 access 성능 측정 (즉, Disk I/O는 어느정도 의미 있으나 Filesystem을 고려하지 않음)
특정 경로 또는 장치명이 어떤 Hard disk 모델인지 확인하는 방법
특정 경로(mount entry) 인 경우
우선 "gbox stat {mount entry}"명령으로 실제 device entry 정보를 확인
확인된 device entry로 다시 인자로 주어 실행 "gbox stat {device entry}"
ATA-MODEL-NAME으로 표시되는 항목이 해당 디스크 모델임.

예시 #1
<pre>
$ gbox stat /ferret
File:                     "/ferret"
File type:                block device
I-node number:            2
Mode:                     40755 (octal)
Link count:               61
Ownership:                UID=0   GID=0
Preferred I/O block size: 4096 bytes
File size:                4096 bytes
Blocks allocated:         8
Last status change:       Wed Mar 20 12:10:58 2019
Last file access:         Wed Mar 20 11:39:00 2019
Last file modification:   Wed Mar 20 12:10:58 2019
dev/rdev:                 8.1 / 0.0
device entry:             /dev/sda1 => 장치명은 /dev/sda 임을 알 수 있음.
mount device entry:       /dev/sda1
filesystem:               ext3
mount options:            rw,relatime,errors=continue,user_xattr,acl,barrier=1,data=ordered
BLKGETSIZE64:             0

$ gbox stat /dev/sda
File:                     "/dev/sda"
File type:                block device
I-node number:            14437
Mode:                     60644 (octal)
Link count:               1
Ownership:                UID=0   GID=0
Preferred I/O block size: 1024 bytes
File size:                0 bytes
Blocks allocated:         0
Last status change:       Wed Mar 20 04:30:26 2019
Last file access:         Wed Mar 20 16:52:46 2019
Last file modification:   Wed Mar 20 04:30:26 2019
dev/rdev:                 1.0 / 8.0
device entry:             /dev/sda
BLKGETSIZE:               500118192
BLKSSZGET:                512
BLKGETSIZE64:             256060514304
ATA-TYPE:                 disk
ATA-MODEL-NAME:           TS256GSSD370S
ATA-SERIAL-NUMBER:        D683360748
ATA-FIRMWARE-REVISION:    O1225G
</pre>

예시 #2 (간편하게 확인하는 방법)
<pre>
$ gbox ferretinfo | grep ATA-MODEL-NAME
KERNEL-disk-ATA-MODEL-NAME="InnoDisk_Corp._iCF_1ME_8GB"
CONF-disk-ATA-MODEL-NAME="InnoDisk_Corp._iCF_1ME_8GB"
LOG-disk-ATA-MODEL-NAME="TS256GSSD370S"
APP-disk-ATA-MODEL-NAME="InnoDisk_Corp._iCF_1ME_8GB"
</pre>
