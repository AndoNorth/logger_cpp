# We use multi-stage buid to flatten the layers afterwards. See trick explanation
# here : https://stackoverflow.com/questions/22713551/how-to-flatten-a-docker-image
# and here : https://docs.docker.com/develop/develop-images/multistage-build/
# We import messir-mss-dependencies environment to build messir-mss
FROM coroborsystems/messir-mss-dependencies:latest as build


USER root:root


#------------------------------------------------------------------------------
RUN echo "Installing mdbook"
RUN cd /tmp ; \
    wget https://github.com/rust-lang/mdBook/releases/download/v0.4.18/mdbook-v0.4.18-x86_64-unknown-linux-gnu.tar.gz ; \
    tar -xzvf mdbook-v0.4.18-x86_64-unknown-linux-gnu.tar.gz ; \
    mv mdbook /usr/bin/ ; \
    wget https://github.com/HollowMan6/mdbook-pdf/releases/download/v0.1.2/mdbook-pdf-v0.1.2-x86_64-unknown-linux-gnu.zip ; \
    unzip mdbook-pdf-v0.1.2-x86_64-unknown-linux-gnu.zip ; \
    mv mdbook-pdf /usr/bin/ ; \
    chmod 755 /usr/bin/mdbook*
    

#------------------------------------------------------------------------------
RUN echo "Building messir-mss"
RUN mkdir /root/messir-mss
COPY . /root/messir-mss

RUN cd /root/messir-mss ; \
    cmake -B .build ; \
    cmake -B .build ; \
    cd .build ; \
    cmake --build . --config RelWithDebInfo; \
    cmake --install . --config RelWithDebInfo


# FROM coroborsystems/messir-mss-base:v10.0.0
FROM coroborsystems/messir-mss-base:latest



#------------------------------------------------------------------------------
RUN echo "Setting technical environment variables"

ENV DEBIAN_FRONTEND noninteractive

ARG GIT_TAG_REV_ARG
ENV GIT_TAG_REV=${GIT_TAG_REV_ARG:-xxxxxxx}

ENV TMP_DIR=/tmp/corobor
ENV COROBOR_HOME=/opt/meteo
ENV COROBOR_CONFIG=$COROBOR_HOME/config

# Actual ODB settings used for this Dockerfile script
# i.e. this is different from MSS ODB_* env. variables
# that are used by messir-mss
ENV DB_HOST=odb
ENV DB_PORT=5432
ENV DB_NAME="odb"
ENV DB_USER="comm_admin"
ENV DB_PASSWORD="meteo@123"



#------------------------------------------------------------------------------
RUN echo "Setting Messir-MSS environment variables"

ENV IS_DOCKER_CTX=1
ENV AppPath=${COROBOR_HOME}
ENV FileQueuePath=${ArchivePath}/FileQueue/
ENV SpecialDefs="hot_standby mem_messages long_socket_filenames"
ENV StorageDuration=365
ENV IsODB=1
ENV IsLDB=0
ENV IsDAL=0
ENV UseLDB=0
# ODB connection
ENV ODB_HOST=${DB_HOST}
ENV ODB_PORT=${DB_PORT}
ENV ODB_NAME=${DB_NAME}
ENV ODB_USER=${DB_USER}
ENV ODB_PASSWORD=${DB_PASSWORD}
ENV OdbMaxAttempts=4
ENV ProxyPort=30100
ENV MaxNumMsgsReturned=8000
ENV MaxNumMsgsInMem=100000
ENV StartupDelay=5
ENV CriticalWarningIds="11028 11064 11025"
ENV GTSid=XXXX
ENV AFTNid=XXXXYMYX
ENV DuplicateCheckLevel=9000
ENV MaxMsgToResend=300
ENV MaxConnPerIp=200
ENV MaxClockDelta=3
# CorrectionQueueSize=500
# BUFR section
ENV CenterId=255
ENV SubCenterId=0
ENV BUFRTableVersion=14
ENV OffsetGMT=0
ENV UseKnot=1
# E-mail section
# ENV SMPTServer=':'@
# ENV SMTPid=
###############################################
#check this
ENV Title="MESSIR-COMM"
ENV Title1=
# First Ethernet Interface
ENV Eth=ens160



#------------------------------------------------------------------------------
RUN echo "Setting MessirComm autostart"
# Variable used in supervisord.conf to decide if mss should be automatically 
# started on container creation False by default : handled by messir-mss-guard
ENV MSS_AUTO_START=false

RUN echo "\"${DB_USER}\" \"${DB_PASSWORD}\"" > ${COROBOR_CONFIG}/userlist.txt



#------------------------------------------------------------------------------
RUN echo "Setting up CommonPreferences.txt"
# Substitute settings with actual value, in CommonPreferences.txt.template, save
# as CommonPreferences.txt
RUN sed \
    -e "s|ODB_PASSWORD|$ODB_PASSWORD|" \
    -e "s|ODB_USER|$DB_USER|" \
    -e "s|ODB_HOST|$DB_HOST|" \
    -e "s|ODB_NAME|$DB_NAME|" \
    ${COROBOR_CONFIG}/CommonPreferences.txt.template > ${COROBOR_CONFIG}/CommonPreferences.txt



#------------------------------------------------------------------------------
RUN echo "Copying resulting binaries"
COPY --from=build /root/messir-mss_install/bin/* ${COROBOR_HOME}/bin/
COPY --from=build /root/messir-mss_install/lib/*.so ${COROBOR_HOME}/lib/
# We don't need to copy anything from dependencies binaries :
# depenencies are built as static libs, thus embeded in the resulting mss binaries
# COPY --from=build /root/messir-mss-dependencies_install/bin/* ${COROBOR_HOME}/bin/



#------------------------------------------------------------------------------
RUN echo "Setting up libraries path"
ENV LD_LIBRARY_PATH=${COROBOR_HOME}/lib:/opt/isode/lib:${LD_LIBRARY_PATH}
RUN ldconfig



#------------------------------------------------------------------------------
RUN echo "Set actual ODB_PORT from Messir-MSS point of view"
ENV ODB_PORT=${DB_PORT}



#------------------------------------------------------------------------------
RUN echo "Setting up directory permissions"
RUN chown -R corobor:root ${COROBOR_HOME} ; \
    chown -R corobor:root ${ARCHIVE_HOME} ; \
    chown -R corobor:root ${SHARED_DATA_HOME} ; \
    chown -R corobor:root ${CONTENT_PICTURES_HOME} ; \
    chown -R corobor:root ${TAF_VERIFICATION_HOME} ; \
    chown -R corobor:root ${BIRDTAM_HOME} ; \
    chmod -R g+rwX ${COROBOR_HOME} ; \
    chmod -R g+rwX ${ARCHIVE_HOME} ; \
    find ${COROBOR_HOME}/ -type f -name "*.sh" -exec chmod 711 {} \;



WORKDIR ${COROBOR_HOME}
USER corobor


EXPOSE 2507/tcp
EXPOSE 2508/tcp
EXPOSE 8092/tcp
EXPOSE 30100/tcp

# We have to do set `SUPPRESS_ISODE_ID_GENERATION=Yes` because, when installing isdbase
# `/etc/isode/isode-id.dat` fails to be created with error "Failed to read system UUID",
# when the package runs the isode command `/opt/isode/sbin/generate_isode_id`. This is
# possibly a docker permissions issues as this issue only happens when calling
# `docker build` inside bitbucket-pipeline: no issue when running 
# `docker build --no-cache --progress plain .` locally, as a simple user. Also see
# messir-mss-base/Dockerfile.
# Consequently, we have to mannually run `/opt/isode/sbin/generate_isode_id` on 
# `messir-mss` docker image startup (see `messir-mss/Dockerfile`).
CMD bash -c ' \
if [ "${MSS_ENABLE_AMHS}" ] \
  echo "Enabling AMHS" \
  /opt/isode/sbin/generate_isode_id \
  mv ${COROBOR_HOME}/lib/libCommObjects.so ${COROBOR_HOME}/lib/libCommObjects_NoAMHS.so \
  mv ${COROBOR_HOME}/lib/libCommObjects-AMHS.so ${COROBOR_HOME}/lib/libCommObjects.so \
fi'; supervisord -n
