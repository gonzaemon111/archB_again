SimPipe: A Simple Mips Pipeline Simulator

1. �͂��߂�

SimPipe�͑���c��w�ŊJ�����ꂽMIPS�p�C�v���C���ƃf�[�^�L���b�V����
�V�~�����[�V�������s���v���O�����ł��B
�{�v���O�����͓��H��ŊJ�����ꂽMIPS���߃Z�b�g�V�~�����[�^SimMips��
��œ��삵�܂��B

2. �R���p�C��

Intel Linux, Cygwin, Mac OS X 10.6.5��ł̓�����m�F���Ă��܂��B
Big-endian�Ȋ��ł͓��삵�܂���B

SimPipe-0.1.4.tgz�̃A�[�J�C�u����肵���ꍇ�͈ȉ��̂悤�ɂ��܂��B
$ tar xvzf SimPipe-0.1.4.tgz
$ cd SimPipe-0.1.4
$ make

3. �R�}���h���C���I�v�V����

�ȉ��̃R�}���h���C���I�v�V���������p�\�ł��B

-dcache-size [num]
    �f�[�^�L���b�V���̃T�C�Y���L���o�C�g�P�ʂŎw�肵�܂�
    �f�t�H���g��1�L���o�C�g�ł�
-dcache-way [num]
�@�@�f�[�^�L���b�V���̃E�F�C�����w�肵�܂�
    �f�t�H���g��1�ł�
-dcache-line [num]
    �f�[�^�L���b�V���̃��C���T�C�Y���o�C�g�P�ʂŎw�肵�܂�
    �f�t�H���g��16�o�C�g�ł�
-dcache-penalty [num]
    �f�[�^�L���b�V���̃~�X�q�b�g�y�i���e�B���w�肵�܂�
    �f�t�H���g��10�T�C�N���ł�
-dcache-writeback [01]
    �f�[�^�L���b�V���̏������݃|���V�[�Ń��C�g�o�b�N�ƃ��C�g�X���[���w�肵�܂�
    �f�t�H���g�̓��C�g�X���[�ł�
-f[01]: Disable forwarding [0] or Enable forwarding [1]
    �t�H���[�f�B���O�̗L�����w�肵�܂�
    �f�t�H���g�̓t�H���[�f�B���O����ł�
-l
    �p�C�v���C���̏󋵂�pipe.log�Ƃ����t�@�C���ɐ������܂�

�Ȃ��A�f�[�^�L���b�V���̓f�t�H���g�ł̓I�t�ɂȂ��Ă���A
�S�Ẵ������A�N�Z�X��1�T�C�N���œ��삵�܂��B
�f�[�^�L���b�V���֌W�̃I�v�V��������ȏ�w�肷��ƁA
�L���b�V�����I���ɂȂ�܂��B

4. ChangeLog

v0.1.4 2016-09-08
- compulsory miss�̌v�Z���@���蔲���������̂𒼂���
  �������Awrite-through�̏ꍇ�̃~�X���ނ̎蔲���͖��Ή�
- branch�n���߂̃t�H���[�f�B���O�̃��f����computer organization�̂��̂ɍ��킹��
  ���Ȃ킿�Aid��branch�n���߂�����ꍇ�͐�s��ALU�n���߂�RAW������ꍇ��1�T�C�N���A
  load�n���߂�RAW������ꍇ��2�T�C�N���̃X�g�[����������

v0.1.3 2010-12-27
- ���[�h���߂̃t�H���[�f�B���O�Ɋւ���o�O���C������
-- �ȉ��̂悤�Ȗ��߃V�[�P���X���l����
--- (1) addu $2,$3,$2
--- (2) lw   $2,0($2)
--- (3) addu $2,$4,$2
-- ���̂Ƃ��A(3)��(2)�Ń��[�h���ꂽ�l��ǂ܂Ȃ���΂Ȃ�Ȃ��̂�(3)��1�T�C�N���X�g�[������K�v������
-- �������Ȃ���A(1)�̃t�H���[�f�B���O������ėL���ƂȂ�X�g�[�������ƂȂ��Ă��܂��Ă���
-- �����ŁA���[�h���߂�E�X�e�[�W��ʉ߂���ۂɐ�s���߂̃t�H���[�f�B���O�𖳌��ɂ���悤�Ȏd�g�݂������

v0.1.2 2010-11-15
- �L���b�V�����C���̃f�t�H���g�l��16�o�C�g�ɂ���

v0.1.1 2009-12-28
- ���W�X�^�t�@�C���֘A�̃o�O���C������
-- ���W�X�^�̃��b�N��^�U�l����A�b�v�E�_�E���J�E���^�ŊǗ�����悤�ɂ���
-- E�X�e�[�W��WRITE_RRA�����������߂̃t�H���[�f�B���O���s���悤�ɂ���
-- WRITE_RD��WRITE_RD_COND�����̃��b�N�ƃt�H���[�f�B���O�̏������Ԉ���Ă����̂��C������
- ���O�t�@�C���Ńt�F�b�`�X�e�[�W�ɐi���������߂̃A�h���X�𐳂����o�͂���悤�ɂ���
- README.txt��������

v0.1.0 2009-12-02
- ���J
