/* Work in progress
 * - Currently, this is only able to send a login packet to the drone.
 * - This will be expanded to allow for all required communications with the drone to enable video output.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <openssl/aes.h>

/* Macros */
#define DRONE_IP                    "172.19.10.1"
#define DRONE_PORT                  8866

#define USERNAME                    "guanxukeji"
#define PASSWORD                    "gxrdw60"
#define FH_AES_KEY                  "guanxukj@fh8620"

#define PACKET_HDR_LEN              82
#define PACKET_PLAINTEXT_LEN        96
#define PACKET_USERNAME_LEN         32
#define PACKET_PASSWORD_LEN         36


/* @brief Constructs a packet header.
 * @param[in] buf Pointer to a buffer used to construct the packet.
 * @param[out] hdr_len Returns the length of the header. Otherwise, -1 on failure.
 */
static int build_packet_header(uint8_t *buf)
{
    if(buf == NULL){
        return -1;
    }
    memset(buf, 0, PACKET_PLAINTEXT_LEN);

    buf[0]      = 0x00;                                                 /* device_type */
    buf[1]      = 0x51;                                                 /* g_ucHeadLen */
    buf[2]      = 0x00;                                                 /* zero'd */
    buf[4]      = 0x01;
    buf[9]      = 0x00;
    strncpy((char *)&buf[10], USERNAME, PACKET_USERNAME_LEN);
    strncpy((char *)&buf[42], PASSWORD, PACKET_PASSWORD_LEN);
    buf[78]     = 0x00;
    buf[79]     = 0x00;
    buf[80]     = 0x01;

    return 81;
}

/* @brief Parent constructor for the packet; adds expected header to the packet in front of the encrypted ciphertext.
 * @param[in] ciphertext Pointer to a buffer containing the encrypted ciphertext.
 * @param[in] cipherlen Length of the ciphertext being sent.
 * @param[in] plaintext_len Length of the plaintext.
 * @param[in] wirepacket Pointer to an output buffer from which the packet is sent.
 */
static int build_wire_packet(uint8_t *ciphertext, int cipher_len, int plaintext_len, uint8_t *wirepacket)
{
    wirepacket[0] = 0x49;
    wirepacket[1] = 0x54;
    int last_block_offset = ((plaintext_len - 1) / 16) * 16;
    int iVar3 = last_block_offset + 20;

    memcpy(&wirepacket[2], &iVar3, 4);
    memcpy(&wirepacket[6], &plaintext_len, 4);
    memcpy(&wirepacket[10], &ciphertext, cipher_len);
    return iVar3 + 6;
}

/* @brief Adds the login command to a configured packet.
 * @param[in] buf Pointer to a buffer used to hold the payload.
 * @param[out] len Returns the length of the packet. Otherwise, -1 on failure.
 */
static int build_login(uint8_t *buf)
{
    if(buf == NULL){
        return -1;
    }

    if((build_packet_header(buf)) == -1){
        return -1;
    }

    buf[3]      = 0x01;                                                 /* cmd_id */
    buf[81]     = 0x00;
    buf[82]     = 0x00;

    return 83;
}

/* @brief Generalized packet constructor for commands.
 * @param[in] buf Pointer to packet header being configured.
 * @param[in] cmd_id Single byte command being passed.
 * @param[in] a10 Unidentified purpose; always 0x00 for login.
 * @param[in] payload_buf Payload buffer for data.
 * @param[in] payload_len Payload buffer length.
 * @param[out] packet_len Returns 83, the length of the header. Otherwise, -1 on failure.
 */
static int build_command_packet(uint8_t, *buf, uint8_t cmd_id, uint8_t a10, uint8_t *payload_buf, int payload_len)
{
    if(buf == NULL){
        return -1;
    }

    if((build_packet_header(buf)) == -1){
        return -1;
    }

    buf[3]      = cmd_id;                                                 /* cmd_id */
    buf[81]     = a10;
    memcpy(&buf[82], &payload_buf, payload_len)

    return 83;
}

/* @brief AES-ECB Encryption is expected by the drone.
 * @param[in] plaintext Pointer to a buffer of plaintext to be encrypted.
 * @param[in] length Length of the plaintext being encrypted.
 * @param[in] key Pointer to an AES key used to encrypt the plaintext.
 * @param[in] ciphertext Pointer to an output buffer for the encrypted ciphertext.
 * @param[out] 96, the length of the plaintext and ciphertext.
 */
static int aes_ecb_encrypt(const uint8_t *plaintext, int length, const uint8_t *key, uint8_t *ciphertext)
{
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);

    int blk_cnt = (length + 15 ) / 16;
    for(int i = 0; i < blk_cnt; i++){
        AES_ecb_encrypt(plaintext + i*16, ciphertext+ i*16, &aes_key, AES_ENCRYPT);
    }
    return blk_cnt * 16;
}

/* @brief AES-ECB Decryption used to decrypt the responses from the drone.
 * @param[in] ciphertext Pointer to a buffer containing the encrypted response from the drone.
 * @param[in] length Length of the ciphertext.
 * @param[in] key AES key used to decrypt the ciphertext.
 * @param[in] plaintext Pointer to an output buffer for the decrypted ciphertext.
 */
static void aes_ecb_decrypt(const uint8_t *ciphertext, int length, const uint8_t *key, uint8_t *plaintext)
{
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);

    int blk_cnt = length / 16;
    for(int i = 0; i < blk_cnt; i++){
        AES_ecb_encrypt(ciphertext + i*16, plaintext + i*16, &aes_key, AES_DECRYPT);
    }
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;

    servaddr.sin_family         = AF_INET;
    servaddr.sin_family.s_addr  = DRONE_IP;
    servaddr.sin_port           = htons(DRONE_PORT);

    if(connect(fd, (struct sockaddr_in *)&servaddr, sizeof(servaddr)) == -1){
        fprintf(stderr, "[ERROR] %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    uint8_t plaintext[96] = { 0 };
    int plaintext_len = build_login(plaintext);

    if(plaintext_len == -1){
        fprintf(stderr, "[ERROR] The plaintext has an incorrect length, or the buffer was NULL.\n");
        return EXIT_FAILURE;
    }

    uint8_t ciphertext[96] = { 0 };
    int ciphertext_len = aes_ecb_encrypt(plaintext, plaintext_len, (uint8_t *)FH_AES_KEY, ciphertext);

    if(ciphertext_len == -1){
        fprintf(stderr, "[ERROR] The ciphertext has an incorrect length, or the buffer was NULL.\n");
        return EXIT_FAILURE;
    }

    uint8_t wirepacket[256] = { 0 };
    int wirepacket_len = build_wire_packet(ciphertext, ciphertext_len, plaintext_len, wirepacket);

    if(wirepacket_len == -1){
        fprintf(stderr, "[ERROR] The wirepacket has an incorrect length, or the buffer was NULL.\n");
        return EXIT_FAILURE;
    }

    if(send(fd, wirepacket, wirepacket_len, 0) != wirepacket_len){
        fprintf(stderr, "[ERROR] %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    uint8_t resp[512] = { 0 };
    ssize_t n = recv(fd, resp, sizeof(resp), 0);
    if(n > 0){
        uint8_t resp_plain[96] = { 0 };
        aes_ecb_decrypt(resp + 10, PACKET_PLAINTEXT_LEN, (uint8_t *)FH_AES_KEY, resp_plain);
        printf("Device type byte: 0x%02x\n", resp_plain[0]);
        printf("Response cmd_id:  0x%02x\n", resp_plain[3]);
        printf("Response seq_id:  0x%02x\n", resp_plain[4]);
        printf("Payload byte:     0x%02x\n", resp_plain[82]);
    } else {
        fprintf(stderr, "[ERROR] %s\n (no response.)", strerror(errno));
    }

    close(fd);

    return 0;
}
