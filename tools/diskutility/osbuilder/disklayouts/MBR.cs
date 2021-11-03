using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace OSBuilder.DiskLayouts
{
    public class MBR : IDiskScheme
    {
        static readonly int MAX_PARTITONS = 4;

        private Disk _disk = null;
        private List<FileSystems.IFileSystem> _fileSystems = new List<FileSystems.IFileSystem>();
        private ulong _sectorsAllocated = 1; // MBR

        public bool Open(Disk disk)
        {
            // Store disk
            _disk = disk;

            // TODO not supported
            return false;
        }

        /**
         * Initializes a disk with the disk-scheme and formats the disk
         * for usage with <AddPartition>, this action wipes the disk
         */
        public bool Create(Disk disk)
        {
            _disk = disk;

            if (!File.Exists("deploy/mbr.sys"))
            {
                Console.WriteLine("Format - MBR scheme requested but mbr.sys is not found (deploy/mbr.sys)");
                return false;
            }

            // ensure disk is open for read/write
            var diskAccessible = _disk.Open();
            if (!diskAccessible)
                return false;

            return true;
        }

        private void SetPartitionInMBR(byte[] mbr, int partition, FileSystems.IFileSystem fileSystem)
        {
            var byteOffset = 446 + (partition * 16);
            ulong partitionStart = fileSystem.GetSectorStart();
            ulong partitionEnd = fileSystem.GetSectorStart() + fileSystem.GetSectorCount();
            Console.WriteLine("partition " + partition.ToString() + 
                " - start: " + partitionStart.ToString() +
                ", end: " + partitionEnd.ToString());

            // determine data to write
            byte status = (byte)(fileSystem.IsBootable() ? 0x80 : 0x00);
            
            ulong headOfStart = (partitionStart / 63) % 16;
            ulong headOfEnd = (partitionEnd / 63) % 16;

            ushort cylinderOfStart = Math.Min((ushort)(partitionStart / (63 * 16)), (ushort)1023);
            ushort cylinderOfEnd = Math.Min((ushort)(partitionEnd / (63 * 16)), (ushort)1023);

            ulong sectorInCylinderStart = (fileSystem.GetSectorStart() % 63) + 1;
            ulong sectorInCylinderEnd = (fileSystem.GetSectorStart() % 63) + 1;
            Console.WriteLine("partiton CHS start - " + cylinderOfStart.ToString() +
                "/" + headOfStart.ToString() + "/" + sectorInCylinderStart.ToString());
            Console.WriteLine("partiton CHS end   - " + cylinderOfEnd.ToString() +
                "/" + headOfEnd.ToString() + "/" + sectorInCylinderEnd.ToString());

            uint sectorOfStart = fileSystem.GetSectorStart() > uint.MaxValue ? uint.MaxValue : (uint)fileSystem.GetSectorStart();
            uint sectorCount = fileSystem.GetSectorCount() > uint.MaxValue ? uint.MaxValue : (uint)fileSystem.GetSectorCount();

            // Set partition status
            mbr[byteOffset] = status;

            // Set partiton start (CHS), high byte is low byte of cylinder
            mbr[byteOffset + 1] = (byte)headOfStart;
            mbr[byteOffset + 2] = (byte)((byte)((cylinderOfStart >> 2) & 0xC0) | (byte)(sectorInCylinderStart & 0x3F));
            mbr[byteOffset + 3] = (byte)(cylinderOfStart & 0xFF);

            // Set partition type
            mbr[byteOffset + 4] = fileSystem.GetFileSystemType();

            // Set partition end (CHS), high byte is low byte of cylinder
            mbr[byteOffset + 5] = (byte)headOfEnd;
            mbr[byteOffset + 6] = (byte)((byte)((cylinderOfEnd >> 2) & 0xC0) | (byte)(sectorInCylinderEnd & 0x3F));
            mbr[byteOffset + 7] = (byte)(cylinderOfEnd & 0xFF);

            // Set partition start (LBA)
            mbr[byteOffset + 8] = (byte)(sectorOfStart & 0xFF);
            mbr[byteOffset + 9] = (byte)((sectorOfStart >> 8) & 0xFF);
            mbr[byteOffset + 10] = (byte)((sectorOfStart >> 16) & 0xFF);
            mbr[byteOffset + 11] = (byte)((sectorOfStart >> 24) & 0xFF);

            // Set partition size (LBA)
            mbr[byteOffset + 12] = (byte)(sectorCount & 0xFF);
            mbr[byteOffset + 13] = (byte)((sectorCount >> 8) & 0xFF);
            mbr[byteOffset + 14] = (byte)((sectorCount >> 16) & 0xFF);
            mbr[byteOffset + 15] = (byte)((sectorCount >> 24) & 0xFF);
        }

        private void SetPartitonEntryEmptyInMBR(byte[] mbr, int partition)
        {
            mbr[446 + (partition * 16)] = 0x00;
            mbr[446 + (partition * 16) + 4] = 0x00;
        }

        /**
         * Finalizes a disk opened or created by Open/Create
         */
        public bool Finalize()
        {
            if (_disk == null)
                return false;

            // Load up mbr and build the partition table
            Console.WriteLine("Finalize - loading mbr (mbr.sys)");
            byte[] mbr = File.ReadAllBytes("deploy/mbr.sys");
            
            // Install partitions in the MBR
            for (int i = 0; i < 4; i++)
            {
                if (i < _fileSystems.Count)
                {
                    SetPartitionInMBR(mbr, i, _fileSystems[i]);
                }
                else
                {
                    SetPartitonEntryEmptyInMBR(mbr, i);
                }
            }

            _disk.Write(mbr, 0, true);
            _disk.Close();
            return true;
        }

        /**
         * Adds a new partition of the filesystem given, with the given size
         */
        public bool AddPartition(FileSystems.IFileSystem fileSystem, ulong sectorCount)
        {
            ulong partitionSize = sectorCount;
            if (_disk == null || fileSystem == null || _fileSystems.Count == MAX_PARTITONS)
                return false;

            if (GetFreeSectorCount() < partitionSize)
            {
                partitionSize = GetFreeSectorCount();
                Console.WriteLine("AddPartition - not enough space, reducing partition size to " + partitionSize + " sectors");
            }

            // Initialize the file-system
            if (!fileSystem.Initialize(_disk, _sectorsAllocated, partitionSize))
                return false;

            // Add sectors allocated
            _fileSystems.Add(fileSystem);
            _sectorsAllocated += partitionSize;
            return fileSystem.Format();
        }

        /** 
         * Retrieves the number of free sectors available for the next partition
         */
        public ulong GetFreeSectorCount()
        {
            if (_disk == null)
                return 0;
            return _disk.TotalSectors - _sectorsAllocated;
        }
    }
}