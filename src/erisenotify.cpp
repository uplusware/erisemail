/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "erisenotify.h"
#include "postnotify.h"

int main(int argc, char* argv[])
{   
    return mta_post_notify(true);
}