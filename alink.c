#include "alink.h"

char case_sensitive=1;
char padsegments=0;
char mapfile=0;
PCHAR mapname=0;
unsigned short maxalloc=0xffff;
int output_type=OUTPUT_EXE;
PCHAR outname=0;

UINT max_segs=DEF_SEG_COUNT,
	max_names=DEF_NAME_COUNT,
	max_grps=DEF_GRP_COUNT,
	max_relocs=DEF_RELOC_COUNT,
	max_imports=DEF_IMP_COUNT,
	max_exports=DEF_EXP_COUNT,
	max_publics=DEF_PUB_COUNT,
	max_externs=DEF_EXT_COUNT;

FILE *afile=0;
UINT filepos=0;
long reclength=0;
unsigned char rectype=0;
char li_le=0;
UINT prevofs=0;
long prevseg=0;
long gotstart=0;
RELOC startaddr;
UINT imageBase=0;
UINT fileAlign=1;
UINT objectAlign=1;
UINT stackSize;
UINT stackCommitSize;
UINT heapSize;
UINT heapCommitSize;
unsigned char osMajor,osMinor;
unsigned char subsysMajor,subsysMinor;
unsigned int subSystem;
int buildDll=FALSE;

long errcount=0;

unsigned char buf[65536];
PDATABLOCK lidata;

PPCHAR namelist;
PPSEG seglist;
PPSEG outlist;
PPGRP grplist;
PPPUBLIC publics;
PPEXTREC externs;
PPRELOC relocs;
PPIMPREC impdefs;
PPEXPREC expdefs;
PPLIBFILE libfiles;
PCHAR modname[256];
PCHAR filename[256];
UINT namecount=0,namemin=0,
	segcount=0,segmin=0,outcount=0,baseSeg=-1,
	grpcount=0,grpmin=0,
	pubcount=0,pubmin=0,
	extcount=0,extmin=0,
	fixcount=0,fixmin=0,
	impcount=0,impmin=0,impsreq=0,
	expcount=0,expmin=0,
	nummods=0,
	filecount=0,
	libcount=0;
	
void processArgs(int argc,char *argv[])
{
	long i,j;
	int helpRequested=FALSE;
	UINT setbase,setfalign,setoalign;
	UINT setstack,setstackcommit,setheap,setheapcommit;
	unsigned char setsubsys;
	int gotbase=FALSE,gotfalign=FALSE,gotoalign=FALSE,gotsubsys=FALSE;
	int gotstack=FALSE,gotstackcommit=FALSE,gotheap=FALSE,gotheapcommit=FALSE;
	char *p;

	for(i=1;i<argc;i++)
	{
		if(argv[i][0]==SWITCHCHAR)
		{
			if(strlen(argv[i])<2)
			{
				printf("Invalid argument \"%s\"\n",argv[i]);
				exit(1);
			}
			switch(argv[i][1])
			{
			case 'c':
				if(strlen(argv[i])==2)
				{
					case_sensitive=1;
					break;
				}
				else if(strlen(argv[i])==3)
				{
					if(argv[i][2]=='+')
					{
						case_sensitive=1;
						break;
					}
					else if(argv[i][2]=='-')
					{
						case_sensitive=0;
						break;
					}
				}
				printf("Invalid switch %s\n",argv[i]);
				exit(1);
				break;
			case 'p':
				switch(strlen(argv[i]))
				{
				case 2:
					padsegments=1;
					break;
				case 3:
					if(argv[i][2]=='+')
					{
						padsegments=1;
						break;
					}
					else if(argv[i][2]=='-')
					{
						padsegments=0;
						break;
					}
				default:
					printf("Invalid switch %s\n",argv[i]);
					exit(1);
				}
				break;
			case 'm':
				switch(strlen(argv[i]))
				{
				case 2:
					mapfile=1;
					break;
				case 3:
					if(argv[i][2]=='+')
					{
						mapfile=1;
						break;
					}
					else if(argv[i][2]=='-')
					{
						mapfile=0;
						break;
					}
				default:
					printf("Invalid switch %s\n",argv[i]);
					exit(1);
				}
				break;
			case 'o':
				switch(strlen(argv[i]))
				{
				case 2:
					if(i<(argc-1))
					{
						i++;
						if(!outname)
						{
							outname=malloc(strlen(argv[i])+1);
							if(!outname)
							{
								ReportError(ERR_NO_MEM);
							}
							strcpy(outname,argv[i]);
						}
						else
						{
							printf("Can't specify two output names\n");
							exit(1);
						}
					}
					else
					{
						printf("Invalid switch %s\n",argv[i]);
						exit(1);
					}
					break;
				default:
					if(!strcmp(argv[i]+2,"EXE"))
					{
						output_type=OUTPUT_EXE;
						imageBase=0;
						fileAlign=1;
						objectAlign=1;						
						stackSize=0;
						stackCommitSize=0;
						heapSize=0;
						heapCommitSize=0;
					}
					else if(!strcmp(argv[i]+2,"COM"))
					{
						output_type=OUTPUT_COM;
						imageBase=0;
						fileAlign=1;
						objectAlign=1;
						stackSize=0;
						stackCommitSize=0;
						heapSize=0;
						heapCommitSize=0;
					}
					else if(!strcmp(argv[i]+2,"PE"))
					{
						output_type=OUTPUT_PE;
						imageBase=WIN32_DEFAULT_BASE;
						fileAlign=WIN32_DEFAULT_FILEALIGN;
						objectAlign=WIN32_DEFAULT_OBJECTALIGN;
						stackSize=WIN32_DEFAULT_STACKSIZE;
						stackCommitSize=WIN32_DEFAULT_STACKCOMMITSIZE;
						heapSize=WIN32_DEFAULT_HEAPSIZE;
						heapCommitSize=WIN32_DEFAULT_HEAPCOMMITSIZE;
						subSystem=WIN32_DEFAULT_SUBSYS;
						subsysMajor=WIN32_DEFAULT_SUBSYSMAJOR;
						subsysMinor=WIN32_DEFAULT_SUBSYSMINOR;
						osMajor=WIN32_DEFAULT_OSMAJOR;
						osMinor=WIN32_DEFAULT_OSMINOR;
					}
					else if(!strcmp(argv[i]+1,"objectalign"))
					{
						if(i<(argc-1))
						{
							i++;
							setoalign=strtoul(argv[i],&p,0);
							if(p[0]) /* if not at end of arg */
							{
								printf("Bad object alignment\n");
								exit(1);
							}
							if((setoalign<512)|| (setoalign>(256*1048576))
								|| (getBitCount(setoalign)>1))
							{
								printf("Bad object alignment\n");
								exit(1);
							}
							gotoalign=TRUE;
						}
						else
						{
							printf("Invalid switch %s\n",argv[i]);
							exit(1);
						}
					}
					else
					{
						printf("Invalid switch %s\n",argv[i]);
						exit(1);
					}
					break;
				}
				break;
			case 'h':
			case 'H':
			case '?':
				if(strlen(argv[i])==2)
				{
					helpRequested=TRUE;
				}
				else if(!strcmp(argv[i]+1,"heapsize"))
				{
					if(i<(argc-1))
					{
						i++;
						setheap=strtoul(argv[i],&p,0);
						if(p[0]) /* if not at end of arg */
						{
							printf("Bad heap size\n");
							exit(1);
						}
						gotheap=TRUE;
					}
					else
					{
						printf("Invalid switch %s\n",argv[i]);
						exit(1);
					}
					break;
				}
				else if(!strcmp(argv[i]+1,"heapcommitsize"))
				{
					if(i<(argc-1))
					{
						i++;
						setheapcommit=strtoul(argv[i],&p,0);
						if(p[0]) /* if not at end of arg */
						{
							printf("Bad heap commit size\n");
							exit(1);
						}
						gotheapcommit=TRUE;
					}
					else
					{
						printf("Invalid switch %s\n",argv[i]);
						exit(1);
					}
					break;
				}
				break;
			case 'b':
				if(!strcmp(argv[i]+1,"base"))
				{
					if(i<(argc-1))
					{
						i++;
						setbase=strtoul(argv[i],&p,0);
						if(p[0]) /* if not at end of arg */
						{
							printf("Bad image base\n");
							exit(1);
						}
						if(setbase&0xffff)
						{
							printf("Bad image base\n");
							exit(1);
						}
						gotbase=TRUE;
					}
					else
					{
						printf("Invalid switch %s\n",argv[i]);
						exit(1);
					}
					break;
				}
				else
				{
					printf("Invalid switch %s\n",argv[i]);
					exit(1);
				}
				break;
			case 's':
				if(!strcmp(argv[i]+1,"subsys"))
				{
					if(i<(argc-1))
					{
						i++;
						if(!strcmp(argv[i],"gui")
							|| !strcmp(argv[i],"windows")
							|| !strcmp(argv[i],"win"))
						{
							setsubsys=PE_SUBSYS_WINDOWS;
							gotsubsys=TRUE;
						}
						else if(!strcmp(argv[i],"char")
							|| !strcmp(argv[i],"console")
							|| !strcmp(argv[i],"con"))
						{
							setsubsys=PE_SUBSYS_CONSOLE;
							gotsubsys=TRUE;
						}
						else
						{
							printf("Invalid subsystem id %s\n",argv[i]);
							exit(1);
						}
					}
					else
					{
						printf("Invalid switch %s\n",argv[i]);
						exit(1);
					}
					break;
				}
				else if(!strcmp(argv[i]+1,"stacksize"))
				{
					if(i<(argc-1))
					{
						i++;
						setstack=strtoul(argv[i],&p,0);
						if(p[0]) /* if not at end of arg */
						{
							printf("Bad stack size\n");
							exit(1);
						}
						gotstack=TRUE;
					}
					else
					{
						printf("Invalid switch %s\n",argv[i]);
						exit(1);
					}
					break;
				}
				else if(!strcmp(argv[i]+1,"stackcommitsize"))
				{
					if(i<(argc-1))
					{
						i++;
						setstackcommit=strtoul(argv[i],&p,0);
						if(p[0]) /* if not at end of arg */
						{
							printf("Bad stack commit size\n");
							exit(1);
						}
						gotstackcommit=TRUE;
					}
					else
					{
						printf("Invalid switch %s\n",argv[i]);
						exit(1);
					}
					break;
				}
				else
				{
					printf("Invalid switch %s\n",argv[i]);
					exit(1);
				}
				break;
			case 'f':
				if(!strcmp(argv[i]+1,"filealign"))
				{
					if(i<(argc-1))
					{
						i++;
						setfalign=strtoul(argv[i],&p,0);
						if(p[0]) /* if not at end of arg */
						{
							printf("Bad file alignment\n");
							exit(1);
						}
						if((setfalign<512)|| (setfalign>65536)
							|| (getBitCount(setfalign)>1))
						{
							printf("Bad file alignment\n");
							exit(1);
						}
						gotfalign=TRUE;
					}
					else
					{
						printf("Invalid switch %s\n",argv[i]);
						exit(1);
					}
				}
				else
				{
					printf("Invalid switch %s\n",argv[i]);
					exit(1);
				}
				break;
			case 'd':
				if(!strcmp(argv[i]+1,"dll"))
				{
					buildDll=TRUE;
				}
				else
				{
					printf("Invalid switch %s\n",argv[i]);
					exit(1);
				}
				break;
			default:
				printf("Invalid switch %s\n",argv[i]);
				exit(1);
			}
		}
		else
		{
			filename[filecount]=malloc(strlen(argv[i])+1);
			if(!filename[filecount])
			{
				printf("Insufficient memory\n");
				exit(1);
			}
			memcpy(filename[filecount],argv[i],strlen(argv[i])+1);
			for(j=strlen(filename[filecount]);
				j&&(filename[filecount][j]!='.')&&
				(filename[filecount][j]!=PATH_CHAR);
				j--);
			if((j<0) || (filename[filecount][j]!='.'))
			{
				j=strlen(filename[filecount]);
				/* add default extension if none specified */
				filename[filecount]=realloc(filename[filecount],strlen(argv[i])+5);
				if(!filename[filecount])
				{
					printf("Insufficient memory\n");
					exit(1);
				}
				strcpy(filename[filecount]+j,DEFAULT_EXTENSION);
			}
			filecount++;
		}
	}
	if(helpRequested || !filecount)
	{
		printf("Usage: ALINK [file [file [...]]] [options]\n");
		printf("\n");
		printf("    Each file may be an object file, or a library. If no extension\n");
		printf("    is specified, .obj is assumed. Modules are only loaded from\n");
		printf("    library files if they are required to match an external reference\n");
		printf("    Options and files may be listed in any order, all mixed together.\n");
		printf("\n");
		printf("The following options are permitted:\n");
		printf("\n");
		printf("    -c      Enable Case sensitivity\n");
		printf("    -p      Enable segment padding\n");
		printf("    -m      Enable map file\n");
		printf("    -h      This help list\n");
		printf("    -H      \"\n");
		printf("    -?      \"\n");
		printf("    -o name Choose output file name\n");
		printf("    -oXXX   Choose output format XXX\n");
		printf("        Available options are:\n");
		printf("            COM - MSDOS COM file\n");
		printf("            EXE - MSDOS EXE file\n");
		printf("            PE  - Win32 PE Executable\n");
		printf("\nOptions for PE files:\n");
		printf("    -base addr        Set base address of image\n");
		printf("    -filealign addr   Set section alignment in file\n");
		printf("    -objectalign addr Set section alignment in memory\n");
		printf("    -subsys xxx       Set subsystem used\n");
		printf("        Available options are:\n");
		printf("            console   Select character mode\n");
		printf("            con       \"\n");
		printf("            char      \"\n");
		printf("            windows   Select windowing mode\n");
		printf("            win       \"\n");
		printf("            gui       \"\n");
		exit(0);
	}
	if((output_type!=OUTPUT_PE) &&
		(gotoalign | gotfalign | gotbase | gotsubsys | gotstack | gotstackcommit
		| gotheap | gotheapcommit | buildDll))
	{
		printf("Option not supported for non-PE output formats\n");
		exit(1);
	}
	if(gotstack)
	{
		stackSize=setstack;
	}
	if(gotstackcommit)
	{
		stackCommitSize=setstackcommit;
	}
	if(stackCommitSize>stackSize)
	{
		printf("Stack commit size is greater than stack size, committing whole stack\n");
		stackCommitSize=stackSize;
	}
	if(gotheap)
	{
		heapSize=setheap;
	}
	if(gotheapcommit)
	{
		heapCommitSize=setheapcommit;
	}
	if(heapCommitSize>heapSize)
	{
		printf("Heap commit size is greater than heap size, committing whole heap\n");
		heapCommitSize=heapSize;
	}
	if(gotoalign)
	{
		objectAlign=setoalign;
	}
	if(gotfalign)
	{
		fileAlign=setfalign;
	}
	if(gotbase)
	{
		imageBase=setbase;
	}
	if(gotsubsys)
	{
		subSystem=setsubsys;
	}
}

void matchExterns()
{
	long i,j,k,old_nummods;
	int n;

	do
	{
		for(i=0;i<expcount;i++)
		{
			expdefs[i]->pubnum=-1;
			for(j=0;j<pubcount;j++)
			{
				if(!strcmp(expdefs[i]->int_name,publics[j]->name)
					|| ((case_sensitive==0) &&
					!stricmp(expdefs[i]->int_name,publics[j]->name)))
				{
					/*printf("export %s=public %i\n",expdefs[i]->int_name,j);*/
					expdefs[i]->pubnum=j;
				}
			}
		}
		for(i=0;i<extcount;i++)
		{
			externs[i]->flags=EXT_NOMATCH;
			for(j=0;j<pubcount;j++)
			{
				if(!strcmp(externs[i]->name,publics[j]->name)
					|| ((case_sensitive==0) &&
					!stricmp(externs[i]->name,publics[j]->name)))
				{
					externs[i]->pubnum=j;
					externs[i]->flags=EXT_MATCHEDPUBLIC;
				}
			}
			if(externs[i]->flags==EXT_NOMATCH)
			{
				for(j=0;j<impcount;j++)
				{
					if(!strcmp(externs[i]->name,impdefs[j]->int_name)
						|| ((case_sensitive==0) &&
						!stricmp(externs[i]->name,impdefs[j]->int_name)))
					{
						externs[i]->flags=EXT_MATCHEDIMPORT;
						externs[i]->impnum=j;
						impsreq++;
					}
				}
			}
			if(externs[i]->flags==EXT_NOMATCH)
			{
				for(j=0;j<expcount;j++)
				{
					if(!strcmp(externs[i]->name,expdefs[j]->exp_name)
						|| ((case_sensitive==0) &&
						!stricmp(externs[i]->name,expdefs[j]->exp_name)))
					{
						externs[i]->pubnum=expdefs[j]->pubnum;
						externs[i]->flags=EXT_MATCHEDPUBLIC;
					}
				}
			}
		}
		
		old_nummods=nummods;
		for(i=0;(i<expcount)&&(nummods==old_nummods);i++)
		{
			if(expdefs[i]->pubnum<0)
			{
				for(j=0;(j<libcount)&&(nummods==old_nummods);j++)
				{
					for(k=0;(k<libfiles[j]->numsyms)&&(nummods==old_nummods);k++)
					{
						if(((case_sensitive==0)&&
							(stricmp(libfiles[j]->syms[k]->name,expdefs[i]->int_name)==0))
							|| (strcmp(libfiles[j]->syms[k]->name,expdefs[i]->int_name)==0))
						{
							loadlibmod(libfiles[j],libfiles[j]->syms[k]->modpage);
						}
					}
				}
			}
		}
		for(i=0;(i<extcount)&&(nummods==old_nummods);i++)
		{
			if(externs[i]->flags==EXT_NOMATCH)
			{
				for(j=0;(j<libcount)&&(nummods==old_nummods);j++)
				{
					for(k=0;(k<libfiles[j]->numsyms)&&(nummods==old_nummods);k++)
					{
						/* check we haven't already loaded library module */
						for(n=0;n<libfiles[j]->modsloaded;n++)
						{
							if(libfiles[j]->modlist[n]==libfiles[j]->syms[k]->modpage) break;
						}
						if(n<libfiles[j]->modsloaded) continue; /* skip this symbol if module already loaded */
						if(((case_sensitive==0)&&
							(stricmp(libfiles[j]->syms[k]->name,externs[i]->name)==0))
							|| (strcmp(libfiles[j]->syms[k]->name,externs[i]->name)==0))
						{
							loadlibmod(libfiles[j],libfiles[j]->syms[k]->modpage);
						} 
					}
				}
			}
		}
	} while (old_nummods!=nummods);
}

void combineBlocks()
{
	long i,j;
	for(i=0;i<segcount;i++)
	{
		if(seglist[i]&&((seglist[i]->attr&SEG_ALIGN)!=SEG_ABS))
		{
			switch(seglist[i]->attr&SEG_COMBINE)
			{
			case SEG_PUBLIC:
			case SEG_PUBLIC2:
			case SEG_STACK:
			case SEG_PUBLIC3:
				 for(j=i+1;j<segcount;j++)
				 {
					if((seglist[j]&&((seglist[j]->attr&SEG_ALIGN)!=SEG_ABS)) &&
					  (
						((seglist[i]->attr&SEG_COMBINE)==SEG_STACK) ||
						((seglist[i]->attr&(SEG_ALIGN|SEG_COMBINE|SEG_USE32))==(seglist[j]->attr&(SEG_ALIGN|SEG_COMBINE|SEG_USE32)))
					  ) &&
					  (strcmp(namelist[seglist[i]->nameindex],namelist[seglist[j]->nameindex])==0)
					  )
					{
						combine_segments(i,j);
					}
				 }
				 break;
			case SEG_COMMON:
				 for(j=i+1;j<segcount;j++)
				 {
					if((seglist[j]&&((seglist[j]->attr&SEG_ALIGN)!=SEG_ABS)) &&
					  ((seglist[i]->attr&(SEG_ALIGN|SEG_COMBINE|SEG_USE32))==(seglist[j]->attr&(SEG_ALIGN|SEG_COMBINE|SEG_USE32)))
					  &&
					  (strcmp(namelist[seglist[i]->nameindex],namelist[seglist[j]->nameindex])==0)
					  )
					{
						combine_common(i,j);
					}
				 }
				 break;
			default:
					break;
			}
		}
	}
		
	for(i=0;i<grpcount;i++)
	{
		if(grplist[i])
		{
			for(j=i+1;j<grpcount;j++)
			{
				if(strcmp(namelist[grplist[i]->nameindex],namelist[grplist[j]->nameindex])==0)
				{
					combine_groups(i,j);
				}
			}
		}
	}
}                

void sortSegments()
{
	long i,j,k;
	for(i=0;i<segcount;i++)
	{
		if(seglist[i])
		{
			if((seglist[i]->attr&SEG_ALIGN)!=SEG_ABS)
			{
				seglist[i]->absframe=0;
			}
		}
	}

	outcount=0;
	outlist=malloc(sizeof(PSEG)*segcount);
	if(!outlist)
	{
		ReportError(ERR_NO_MEM);
	}
	for(i=0;i<grpcount;i++)
	{
		if(grplist[i])
		{
			grplist[i]->segnum=-1;
			for(j=0;j<grplist[i]->numsegs;j++)
			{
				k=grplist[i]->segindex[j];
				if(!seglist[k])
				{
					printf("Error - group %s contains non-existent segment\n",namelist[grplist[i]->nameindex]);
					exit(1);
				}
				/* add non-absolute segment of non-zero length */
				if(((seglist[k]->attr&SEG_ALIGN)!=SEG_ABS) &&
					(seglist[k]->length>0))
				{
					if(seglist[k]->absframe!=0)
					{
						printf("Error - Segment %s part of more than one group\n",namelist[seglist[k]->nameindex]);
						exit(1);
					}
					seglist[k]->absframe=1;
					seglist[k]->absofs=i+1;
					if(grplist[i]->segnum<0)
					{
						grplist[i]->segnum=k;
					}
					if(outcount==0) baseSeg=k;
					outlist[outcount]=seglist[k];
					outcount++;
				}
			}
		}
	}
	for(i=0;i<segcount;i++)
	{
		if(seglist[i])
		{
			/* add non-absolute segment of non-zero length */
			if(((seglist[i]->attr&SEG_ALIGN)!=SEG_ABS) &&
				(seglist[i]->length>0))
			{
				if(!seglist[i]->absframe)
				{
					seglist[i]->absframe=1;
					seglist[i]->absofs=0;
					if(outcount==0) baseSeg=i;
					outlist[outcount]=seglist[i];
					outcount++;
				}
			}
			else
			{
				seglist[i]->base=(seglist[i]->absframe<<4)+seglist[i]->absofs;
			}
		}
	}
}                

void alignSegments()
{
	long i,j,k;
	j=0;
	for(i=0;i<outcount;i++)
	{
		if(outlist[i])
		{
			switch(outlist[i]->attr&SEG_ALIGN)
			{
			case SEG_WORD:
			case SEG_BYTE:
			case SEG_DWORD:
			case SEG_PARA:
				k=0x10;
				break;
			case SEG_PAGE:
				k=0x100;
				break;
			case SEG_MEMPAGE:
				k=0x1000;
				break;
			default:
				break;
			}
			if(k<objectAlign) k=objectAlign;
			j=(j+k-1)&(0xffffffff-(k-1));
			outlist[i]->base=j;
			j+=outlist[i]->length;
		}
	}
}

void loadFiles()
{
	long i,j;
	for(i=0;i<filecount;i++)
	{
		afile=fopen(filename[i],"rb");
		if(!afile)
		{
			printf("Error opening file %s\n",filename[i]);
			exit(1);
		}
		filepos=0;
		printf("Loading file %s\n",filename[i]);
		j=fgetc(afile);
		fseek(afile,0,SEEK_SET);
		switch(j)
		{
		case LIBHDR:
			loadlib(afile,filename[i]);
			break;
		case THEADR:
		case LHEADR:
			loadmod(afile);
			break;
		default:
			printf("Unknown file type\n");
			fclose(afile);
			exit(1);
		}
		fclose(afile);
	}
}

void generateMap()
{
	long i;
	afile=fopen(mapname,"wt");
	if(!afile)
	{
		printf("Error opening map file %s\n",mapname);
		exit(1);
	}
	
	for(i=0;i<segcount;i++)
	{
		if(seglist[i])
		{
			fprintf(afile,"SEGMENT %s ",
				(seglist[i]->nameindex>=0)?namelist[seglist[i]->nameindex]:"");
			switch(seglist[i]->attr&SEG_COMBINE)
			{
			case SEG_PRIVATE:
				 fprintf(afile,"PRIVATE ");
				 break;
			case SEG_PUBLIC:
				 fprintf(afile,"PUBLIC ");
				 break;
			case SEG_PUBLIC2:
				 fprintf(afile,"PUBLIC(2) ");
				 break;
			case SEG_STACK:
				 fprintf(afile,"STACK ");
				 break;
			case SEG_COMMON:
				 fprintf(afile,"COMMON ");
				 break;
			case SEG_PUBLIC3:
				 fprintf(afile,"PUBLIC(3) ");
				 break;
			default:
				 fprintf(afile,"unknown ");
				 break;
			}
			if(seglist[i]->attr&SEG_USE32)
			{
				fprintf(afile,"USE32 ");
			}               
			else
			{
				fprintf(afile,"USE16 ");
			}
			switch(seglist[i]->attr&SEG_ALIGN)
			{
			case SEG_ABS:
				 fprintf(afile,"AT 0%04lXh ",seglist[i]->absframe);
				 break;
			case SEG_BYTE:
				 fprintf(afile,"BYTE ");
				 break;
			case SEG_WORD:
				 fprintf(afile,"WORD ");
				 break;
			case SEG_PARA:
				 fprintf(afile,"PARA ");
				 break;
			case SEG_PAGE:
				 fprintf(afile,"PAGE ");
				 break;
			case SEG_DWORD:
				 fprintf(afile,"DWORD ");
				 break;
			case SEG_MEMPAGE:
				 fprintf(afile,"MEMPAGE ");
				 break;
			default:
				 fprintf(afile,"unknown ");
			}
			if(seglist[i]->classindex>=0)
				fprintf(afile,"'%s'\n",namelist[seglist[i]->classindex]);
			else
				fprintf(afile,"\n");
			fprintf(afile,"  at %08lX, length %08lX\n",seglist[i]->base,seglist[i]->length);
		}
	}

	if(pubcount)
	{
		fprintf(afile,"\n %li publics:\n",pubcount);
		for(i=0;i<pubcount;i++)
		{
			fprintf(afile,"%s at %s:%08lX\n",
				publics[i]->name,
				(publics[i]->segnum>=0) ? namelist[seglist[publics[i]->segnum]->nameindex] : "Absolute",
				publics[i]->ofs);
		}
	}        
		
	if(expcount)
	{
		fprintf(afile,"\n %li exports:\n",expcount);
		for(i=0;i<expcount;i++)
		{
			fprintf(afile,"%s(%i)=%s\n",expdefs[i]->exp_name,expdefs[i]->ordinal,expdefs[i]->int_name);
		}
	}
	if(impcount)
	{
		fprintf(afile,"\n %li imports:\n",impcount);
		for(i=0;i<impcount;i++)
		{
			fprintf(afile,"%s=%s:%s(%i)\n",impdefs[i]->int_name,impdefs[i]->mod_name,impdefs[i]->flags==0?impdefs[i]->imp_name:"",    
				impdefs[i]->flags==0?0:impdefs[i]->ordinal);
		}
	}
	fclose(afile);
}

int main(int argc,char *argv[])
{
	long i;

	printf("ALINK v1.0 (C) Copyright 1998 Anthony A.J. Williams.\n");
	printf("All Rights Reserved\n\n");

	processArgs(argc,argv);
	
	if(!filecount)
	{
		printf("No files specified\n");
		exit(1);
	}

	seglist=malloc(max_segs*sizeof(PSEG));
	if(!seglist)
	{
		ReportError(ERR_NO_MEM);
	}
	namelist=malloc(max_names*sizeof(PCHAR));
	if(!namelist)
	{
		ReportError(ERR_NO_MEM);
	}
	grplist=malloc(max_grps*sizeof(PGRP));
	if(!grplist)
	{
		ReportError(ERR_NO_MEM);
	}
	relocs=malloc(max_relocs*sizeof(PRELOC));
	if(!relocs)
	{
		ReportError(ERR_NO_MEM);
	}
	impdefs=malloc(max_imports*sizeof(PIMPREC));
	if(!impdefs)
	{
		ReportError(ERR_NO_MEM);
	}
	expdefs=malloc(max_exports*sizeof(PEXPREC));
	if(!expdefs)
	{
		ReportError(ERR_NO_MEM);
	}
	publics=malloc(max_publics*sizeof(PPUBLIC));
	if(!publics)
	{
		ReportError(ERR_NO_MEM);
	}
	externs=malloc(max_externs*sizeof(PEXTREC));
	if(!externs)
	{
		ReportError(ERR_NO_MEM);
	}
	libfiles=malloc(DEF_LIBFILE_COUNT*sizeof(PLIBFILE));
	if(!libfiles)
	{
		ReportError(ERR_NO_MEM);
	}

	if(!outname)
	{
		outname=malloc(strlen(filename[0])+1+4);
		if(!outname)
		{
			ReportError(ERR_NO_MEM);
		}
		strcpy(outname,filename[0]);
		i=strlen(outname);
		while((i>=0)&&(outname[i]!='.')&&(outname[i]!=PATH_CHAR)&&(outname[i]!=':'))
		{
			i--;
		}
		if(outname[i]!='.')
		{
			i=strlen(outname);
		}
		switch(output_type)
		{
		case OUTPUT_EXE:
		case OUTPUT_PE:
			if(!buildDll)
			{
				strcpy(&outname[i],".exe");
			}
			else
			{
				strcpy(&outname[i],".dll");
			}
			break;
		case OUTPUT_COM:
			strcpy(&outname[i],".com");
			break;
		default:
			break;
		}
	}
		
	if(mapfile)
	{
		if(!mapname)
		{
			mapname=malloc(strlen(outname)+1+4);
			if(!mapname)
			{
				ReportError(ERR_NO_MEM);
			}
			strcpy(mapname,outname);
			i=strlen(mapname);
			while((i>=0)&&(mapname[i]!='.')&&(mapname[i]!=PATH_CHAR)&&(mapname[i]!=':'))
			{
				i--;
			}
			if(mapname[i]!='.')
			{
				i=strlen(mapname);
			}
			strcpy(mapname+i,".map");
		}                        
	}
	else
	{
		if(mapname)
		{
			free(mapname);
			mapname=0;
		}
	}

	loadFiles();
		
	if(!nummods)
	{
		printf("No required modules specified\n");
		exit(1);
	}
		
	matchExterns();

	for(i=0;i<expcount;i++)
	{
		if(expdefs[i]->pubnum<0)
		{
			printf("Unresolved export %s=%s\n",expdefs[i]->exp_name,expdefs[i]->int_name);
			errcount++;
		}
	}

	for(i=0;i<extcount;i++)
	{
		if(externs[i]->flags==EXT_NOMATCH)
		{
			printf("Unresolved external %s\n",externs[i]->name);
			errcount++;
		}
	}
	if(errcount!=0)
	{
		exit(1);
	}
		
	combineBlocks();

	sortSegments();

	alignSegments();

	switch(output_type)
	{
	case OUTPUT_COM:
		OutputCOMfile(outname);
		break;
	case OUTPUT_EXE:
		OutputEXEfile(outname);
		break;
	case OUTPUT_PE:
		OutputWin32file(outname);
		break;
	default:
		printf("Invalid output type\n");
		exit(1);
		break;
	}
	if(mapfile) generateMap();
	return 0;
}
