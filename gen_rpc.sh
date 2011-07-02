#!/bin/sh

cd Common
rpcgen -a tasks_explorer.x
cp tasks_explorer_xdr.c tasks_explorer_xdr_c.c
